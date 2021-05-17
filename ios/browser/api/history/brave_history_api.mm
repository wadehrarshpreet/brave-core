/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_history_api.h"
#include "brave/ios/browser/api/history/brave_history_observer.h"
#include "brave/ios/browser/api/history/brave_browsing_history_driver.h"

#include <unordered_map>

#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/strings/sys_string_conversions.h"
#include "base/guid.h"
#include "base/bind.h"

#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/history/core/browser/browsing_history_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/sync/driver/sync_service.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remover.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remover_factory.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remove_mask.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/web/public/thread/web_thread.h"

#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

# pragma mark - History Transtition Type-Mapping

namespace brave_ios {
namespace {
std::unordered_map<ui::PageTransition, BraveHistoryTransitionType> mapping = {
  {ui::PageTransition::PAGE_TRANSITION_LINK, BraveHistoryTransitionType_LINK},
  {ui::PageTransition::PAGE_TRANSITION_TYPED, BraveHistoryTransitionType_TYPED},
  {ui::PageTransition::PAGE_TRANSITION_AUTO_BOOKMARK, BraveHistoryTransitionType_AUTO_BOOKMARK},
  {ui::PageTransition::PAGE_TRANSITION_AUTO_SUBFRAME, BraveHistoryTransitionType_AUTO_SUBFRAME},
  {ui::PageTransition::PAGE_TRANSITION_MANUAL_SUBFRAME, BraveHistoryTransitionType_MANUAL_SUBFRAME},
  {ui::PageTransition::PAGE_TRANSITION_GENERATED, BraveHistoryTransitionType_GENERATED},
  {ui::PageTransition::PAGE_TRANSITION_AUTO_TOPLEVEL, BraveHistoryTransitionType_TOPLEVEL},
  {ui::PageTransition::PAGE_TRANSITION_FORM_SUBMIT, BraveHistoryTransitionType_FORM_SUBMIT},
  {ui::PageTransition::PAGE_TRANSITION_RELOAD, BraveHistoryTransitionType_RELOAD},
  {ui::PageTransition::PAGE_TRANSITION_KEYWORD, BraveHistoryTransitionType_KEYWORD},
  {ui::PageTransition::PAGE_TRANSITION_KEYWORD_GENERATED, BraveHistoryTransitionType_KEYWORD_GENERATED}
};
} // namespace

// ui::PageTransition page_transition_from_type(BraveHistoryTransitionType type) {
//   ui::PageTransition result = ui::PageTransition::PAGE_TRANSITION_TYPED;

//   // for (auto it = mapping.begin(); it != mapping.end(); ++it) {
//   //   if (it->second == type) {
//   //     return it->first;
//   //   }
//   // }

//   return result;
// }

// BraveHistoryTransitionType type_from_page_transition(const ui::PageTransition& page_transition) {
//   BraveHistoryTransitionType result = BraveHistoryTransitionType_TYPED;

//   // for (auto it = mapping.begin(); it != mapping.end(); ++it) {
//   //   if (result == it->first) {
//   //     return it->second;
//   //   }
//   // }

//   return result;
// }
} // namespace brave_ios

#pragma mark - IOSHistoryNode

@interface IOSHistoryNode () {
  base::string16 title_;
  GURL gurl_;
  base::Time date_added_;
}
@end

@implementation IOSHistoryNode

/// History Node Constructor used with HistoryAPI
/// @param url Mandatory URL field for the history object
/// @param title Title used for the URL
/// @param dateAdded Date History Object is created
- (instancetype)initWithURL:(NSURL*)url
                      title:(NSString* _Nullable)title
                  dateAdded:(NSDate* _Nullable)dateAdded {
  if ((self = [super init])) {
    [self setUrl:url];

    if (title) {
     [self setTitle:title];
    }

    if (dateAdded) {
      [self setDateAdded:dateAdded];
    }
  }

  return self;
}

- (void)setUrl:(NSURL*)url {
  gurl_ = net::GURLWithNSURL(url);
}

- (NSURL*)url {
  return net::NSURLWithGURL(gurl_);
}

- (void)setTitle:(NSString*)title {
  title_ = base::SysNSStringToUTF16(title);
}

- (NSString*)title {
  return base::SysUTF16ToNSString(title_);
}

- (void)setDateAdded:(NSDate*)dateAdded {
  date_added_ = base::Time::FromDoubleT([dateAdded timeIntervalSince1970]);
}

- (NSDate*)dateAdded {
  return [NSDate dateWithTimeIntervalSince1970:date_added_.ToDoubleT()];
}
@end

#pragma mark - BraveHistoryAPI

@interface BraveHistoryAPI ()<BraveHistoryDriverDelegate>  {
  // Browser State Used to retreive History Driver - History Service and Web History Service
  ChromeBrowserState* browser_state_;
  // History Browsing Service Driver
  std::unique_ptr<BraveBrowsingHistoryDriver> browsing_history_driver_;
  // History Browsing Service
  std::unique_ptr<history::BrowsingHistoryService> browsing_history_service_;
  // History Service for adding and querying
  history::HistoryService* history_service_ ;
  // WebhistoryService for remove and elete operations
  history::WebHistoryService* web_history_service_;
  // Tracker for history requests.
  base::CancelableTaskTracker tracker_;
}
@property (nonatomic, strong) void(^query_completion)(NSArray<IOSHistoryNode*>*);
@end

@implementation BraveHistoryAPI

/// Shared Singleton Instance for History API
+ (instancetype)sharedHistoryAPI {
  static BraveHistoryAPI* instance = nil;
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    instance = [[BraveHistoryAPI alloc] init];
  });

  return instance;
}

/// Constructor which is setting up services 
/// like HistoryService / SyncService / BrowsingHistoryService / WebHistoryService
- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    browser_state_ =
        browserStateManager->GetLastUsedBrowserState();
    browsing_history_driver_ = std::make_unique<BraveBrowsingHistoryDriver>(
        browser_state_,
        self);
    history_service_ = ios::HistoryServiceFactory::GetForBrowserState(
        browser_state_, ServiceAccessType::EXPLICIT_ACCESS);
    syncer::SyncService* sync_service =
        ProfileSyncServiceFactory::GetForBrowserState(browser_state_);
    browsing_history_service_ =
        std::make_unique<history::BrowsingHistoryService>(
              browsing_history_driver_.get(),
              history_service_,
              sync_service);
    
    web_history_service_ = ios::WebHistoryServiceFactory::GetForBrowserState(browser_state_);
    DCHECK(history_service_);
    DCHECK(browsing_history_service_);
  }
  return self;
}

- (void)dealloc {
  browser_state_ = nil;
  history_service_ = nil;
  web_history_service_ = nil;
}

- (id<HistoryServiceListener>)addObserver:(id<HistoryServiceObserver>)observer {
  return [[HistoryServiceListenerImpl alloc] init:observer
                                   historyService:history_service_];
}

- (void)removeObserver:(id<HistoryServiceListener>)observer {
  [observer destroy];
}

- (bool)isLoaded {
  // Triggers backend to load if it hasn't already, and then returns whether
  // it's finished loading.
  return history_service_->BackendLoaded();
}

- (void)addHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  history::HistoryAddPageArgs args;
  args.url = net::GURLWithNSURL(history.url);
  args.time = base::Time::FromDoubleT([history.dateAdded timeIntervalSince1970]);
  args.redirects = history::RedirectList();
  args.transition = ui::PAGE_TRANSITION_TYPED; // Important! Page Transition has to be typed or data will not be synced
  args.hidden = false;
  args.visit_source = history::VisitSource::SOURCE_BROWSED;
  args.consider_for_ntp_most_visited = true;
  args.title = base::SysNSStringToUTF16(history.title);
  args.floc_allowed = false; // Disable Floc - Not allow tracking

  history_service_->AddPage(args);
}

- (void)removeHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes a specific URL using history service and web history service
  history_service_->DeleteLocalAndRemoteUrl(web_history_service_,
                                           net::GURLWithNSURL(history.url));
}

- (void)removeAllWithCompletion:(void(^)())completion {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Deletes entire History and from all synced devices
  history_service_->DeleteLocalAndRemoteHistoryBetween(web_history_service_,
                                                      base::Time::Min(),
                                                      base::Time::Max(),
                                                      base::BindOnce([](std::function<void()> completion){
                                                        completion();
                                                      }, completion),
                                                      &tracker_);
}

- (void)searchWithQuery:(NSString*)query maxCount:(NSUInteger)maxCount
                                       completion:(void(^)(NSArray<IOSHistoryNode*>* historyResults))completion {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(history_service_->backend_loaded());

  // Check Query is empty for Fetching all history
  // The entered query can be nil or empty String
  BOOL fetchAllHistory = !query || [query length] > 0;
  std::u16string queryString =
        fetchAllHistory ? std::u16string() : base::SysNSStringToUTF16(query);

  // Creating fetch options for querying history
  history::QueryOptions options;
  options.duplicate_policy =
      fetchAllHistory ? history::QueryOptions::REMOVE_DUPLICATES_PER_DAY
                      : history::QueryOptions::REMOVE_ALL_DUPLICATES;
  options.max_count = fetchAllHistory ? 0 : static_cast<int>(maxCount);
  options.matching_algorithm =
      query_parser::MatchingAlgorithm::ALWAYS_PREFIX_SEARCH;

  _query_completion = completion;

  browsing_history_service_->QueryHistory(queryString, options);
}

#pragma mark - BraveHistoryDriverDelegate

- (void)historyQueryWasCompletedWithResults:
    (const std::vector<history::BrowsingHistoryService::HistoryEntry>&)results
                   queryResultsInfo:(const history::BrowsingHistoryService::
                                         QueryResultsInfo&)queryResultsInfo
                        continuationClosure:(base::OnceClosure)continuationClosure {
  
  if (_query_completion) {
    continuationClosure.Reset();
    
    NSMutableArray<IOSHistoryNode*>* historyNodes = [[NSMutableArray alloc] init];

    for (const auto& result : results) {
      IOSHistoryNode *historyNode = [[IOSHistoryNode alloc] initWithURL:net::NSURLWithGURL(result.url)
                                                                  title:base::SysUTF16ToNSString(result.title)
                                                              dateAdded:[NSDate dateWithTimeIntervalSince1970:
                                                                            result.time.ToDoubleT()]];
      [historyNodes addObject:historyNode];
    }

    _query_completion([historyNodes copy]);
    _query_completion = nullptr;
  } else {
    // Pagination
    std::move(continuationClosure).Run();
  }
}

- (void)historyWasDeleted {}

- (void)showNoticeAboutOtherFormsOfBrowsingHistory:(BOOL)shouldShowNotice {}
@end
