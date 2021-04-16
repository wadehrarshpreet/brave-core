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

#include "ios/web/public/thread/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using namespace base;

@interface IOSHistoryNode () {
  bool owned_;
}
@end

@implementation IOSHistoryNode

- (instancetype)initWithTitle:(NSString*)title
                           id:(int64_t)id
                         guid:(NSString*)guid
                          url:(NSURL*)url
                    dateAdded:(NSDate*)dateAdded {
  if ((self = [super init])) {
    // Title
    string16 title_ = SysNSStringToUTF16(title);

    // TID
    int64_t id_ = static_cast<int64_t>(id);
    DCHECK(id_);

    // UID
    GUID guid_ = GUID();
    if ([guid length] > 0) {
      string16 guid_string = SysNSStringToUTF16(guid);
      DCHECK(IsValidGUID(guid_string));
      guid_ = GUID::ParseCaseInsensitive(guid_string);
    } else {
      guid_ = GUID::GenerateRandomV4();
    }

    // URL
    GURL gurl_ = net::GURLWithNSURL(url);

    // Date Added
    if (dateAdded) {
      // base::Time date_added_ = base::Time::FromDoubleT([dateAdded timeIntervalSince1970]);
    }

    owned_ = true;
  }

  return self;
}

- (void)dealloc {
  if (owned_) {
    owned_ = false;
  }
}

- (void)setTitle:(NSString*)title {

}

- (NSString*)title {
  NSString *title = @"History Title";
  return title;
}

- (NSString*)guid {
  NSString *guid = @"GUID";
  return guid;
}

- (void)setUrl:(NSURL*)url {
    
}

- (NSURL*)url {
  NSString *historyURLString = @"www.brave.com";
  NSURL *testURL = [NSURL URLWithString: historyURLString];

  return testURL;
}

- (NSURL*)iconUrl {
  NSString *iconURLString = @"www.brave.com";
  NSURL *testIconURL = [NSURL URLWithString: iconURLString];

  return testIconURL;
}

- (UIImage*)icon {
  UIImage *testIconImage = [UIImage imageNamed:@"test_img"];
  return testIconImage;
}

- (NSDate*)dateAdded {
  NSDate *testDate= [NSDate date];
  return testDate;
}

- (void)setDateAdded:(NSDate*)date {

}

- (bool)isFavIconLoaded {
  return true;
}

- (bool)isFavIconLoading {
  return true;
}

- (void)remove {

}

@end

@interface BraveHistoryAPI ()  {
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

    DCHECK(browserState);
  }
  return self;
}

- (void)dealloc {
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

- (void)removeHistory:(IOSHistoryNode*)history {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

- (void)removeAll {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

@end
