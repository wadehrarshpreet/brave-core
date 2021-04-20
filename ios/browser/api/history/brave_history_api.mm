/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/brave_history_api.h"

#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/guid.h"
#include "base/strings/sys_string_conversions.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/history/history_service_factory.h"

#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/service_access_type.h"

#include "ios/web/public/thread/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using namespace base;

@interface IOSHistoryNode () {
  string16 title_;
  GUID guid_;
  GURL gurl_;
  base::Time date_added_;
}
@end

@implementation IOSHistoryNode

- (instancetype)initWithTitle:(NSString*)title
                         guid:(NSString*)guid
                          url:(NSURL*)url
                    dateAdded:(NSDate*)dateAdded {
  if ((self = [super init])) {
    // Title
    [self setTitle:title];

    // UID
    guid_ = GUID();
    if ([guid length] > 0) {
      string16 guid_string = SysNSStringToUTF16(guid);
      DCHECK(IsValidGUID(guid_string));
      guid_ = GUID::ParseCaseInsensitive(guid_string);
    } else {
      guid_ = GUID::GenerateRandomV4();
    }

    // URL
    gurl_ = net::GURLWithNSURL(url);

    // Date Added
    date_added_ = base::Time::FromDoubleT([dateAdded timeIntervalSince1970]);
  }

  return self;
}

- (void)dealloc {
}

- (void)setTitle:(NSString*)title {
  title_ = SysNSStringToUTF16(title);
}

- (NSString*)title {
  return base::SysUTF16ToNSString(title_);
}

- (void)setUrl:(NSURL*)url {
  gurl_ = net::GURLWithNSURL(url);
}

- (NSURL*)url {
  return net::NSURLWithGURL(gurl_);
}

- (void)setDateAdded:(NSDate*)dateAdded {
  date_added_ = base::Time::FromDoubleT([dateAdded timeIntervalSince1970]);
}

- (NSDate*)dateAdded {
  return [NSDate dateWithTimeIntervalSince1970:date_added_.ToDoubleT()];
}

@end

@interface BraveHistoryAPI ()  {
  history::HistoryService* history_service_ ;
}
@end

@implementation BraveHistoryAPI
+ (instancetype)sharedHistoryAPI {
  static BraveHistoryAPI* instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[BraveHistoryAPI alloc] init];
  });
  return instance;
}

- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* browserState =
        browserStateManager->GetLastUsedBrowserState();
    history_service_ = ios::HistoryServiceFactory::GetForBrowserState(
              browserState, ServiceAccessType::EXPLICIT_ACCESS);

    DCHECK(history_service_);
  }
  return self;
}

- (void)dealloc {
    history_service_ = nil;
}

- (bool)isLoaded {
    return true;
}

// - (id<HistoryModelListener>)addObserver:(id<HistoryModelObserver>)observer {
// //   return [[HistoryModelListenerImpl alloc] init:observer
// //                                    historyModel:history_model_];
// }

// - (void)removeObserver:(id<HistoryModelListener>)observer {
// //   [observer destroy];
// }

- (void)addHistory:(IOSHistoryNode*)history {
  history::HistoryAddPageArgs args;
  args.url = net::GURLWithNSURL(history.url);
  args.time = base::Time::FromDoubleT([history.dateAdded timeIntervalSince1970]);
  args.visit_source = history::VisitSource::SOURCE_BROWSED;

  history_service_->AddPage(args);
}

- (void)removeHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

- (void)removeAll {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

@end