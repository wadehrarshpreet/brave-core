/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include "base/strings/sys_string_conversions.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#import "brave/ios/browser/api/history/brave_history_api.h"

#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/history_types.h"

#include "third_party/skia/include/core/SkColor.h"

@interface IOSHistoryNode()
{
    // history::PrepopulatedPage* _pre_populated_page;
    std::unique_ptr<history::PrepopulatedPage> _pre_populated_page;
}
@end


@implementation IOSHistoryNode

- (instancetype)init {
    if ((self = [super init])) {
        //CHECK(false);
    }
    return self;
}

- (instancetype) initWithTitle:(NSString *)title withURL:(NSURL *)url {
    if ((self = [super init])) {
        // _pre_populated_page = new PrepopulatedPage()
    
        // _pre_populated_page = std::unique_ptr<PrepopulatedPage>(
        //     url,
        //     title,
        //     favicon_id,
        //     color            
        // )

        // Converting GURL to NSURL
        // GURL gurl_ = net::GURLWithNSURL(url);

        // // Converting base::string to NSString
        // base::string16 gTitle_ = base::SysNSStringToUTF16(title);

        // _pre_populated_page = std::make_unique(
        //     gurl_,
        //     gTitle_,
        //     0 //favicon_id
        //     SkColor //color
        // )
    }
    return self;
}

// - (NSString *)title {
//     return base::SysNSStringToUTF16(_pre_populated_page->title());
// }

// - (void)dealloc {
//     delete _pre_populated_page
// }
@end
