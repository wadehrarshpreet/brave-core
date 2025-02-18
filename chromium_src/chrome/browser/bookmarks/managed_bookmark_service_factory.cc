/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"

#include "src/chrome/browser/bookmarks/managed_bookmark_service_factory.cc"

content::BrowserContext* ManagedBookmarkServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // To make different service for normal and incognito profile.
  return chrome::GetBrowserContextRedirectedInIncognitoOverride(context);
}
