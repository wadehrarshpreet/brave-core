/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

// Public Interface for History Object to be used iOS
OBJC_EXPORT
@interface IOSHistoryNode : NSObject

// Title and URL for the History object
@property (nonatomic, strong, readonly) NSString *title;
@property (nonatomic, strong, readonly) NSURL *url;

// The Date properties
@property (nonatomic, strong, readonly) NSDate *dateAdded;
@property (nonatomic, strong, readonly) NSDate *lastVisitedDate;

// Fav Icon related properties
@property (nonatomic, strong, readonly) UIImage *favIcon;
@property (nonatomic, strong, readonly) NSURL *favIconURL;

@property (nonatomic, strong, readonly) UIColor *skcolor;

// Constructor declaring basic History properties
- (instancetype) init;
- (instancetype) initWithTitle:(NSString *)title withURL:(NSURL *)url;

@end
