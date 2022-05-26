/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_tab_helper.h"

#include <string>
#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/ipfs/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#endif

#include "chrome/browser/profiles/profile.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

namespace brave_rewards {

RewardsTabHelper::RewardsTabHelper(content::WebContents* web_contents)
    : content::WebContentsUserData<RewardsTabHelper>(*web_contents),
      WebContentsObserver(web_contents),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  if (tab_id_.is_valid()) {
    rewards_service_ = RewardsServiceFactory::GetForProfile(
        Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  }

  if (rewards_service_) {
    rewards_service_->AddObserver(this);
  }

#if !BUILDFLAG(IS_ANDROID)
  BrowserList::AddObserver(this);
#endif
}

RewardsTabHelper::~RewardsTabHelper() {
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void RewardsTabHelper::SetPublisherIdForTab(const std::string& publisher_id) {
  if (publisher_id != publisher_id_) {
    publisher_id_ = publisher_id;
    for (auto& observer : observer_list_) {
      observer.OnPublisherUpdated(publisher_id_);
    }
  }
}

void RewardsTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!rewards_service_ || render_frame_host->GetParent()) {
    return;
  }

#if BUILDFLAG(ENABLE_IPFS)
  auto ipns_url = web_contents()->GetLastCommittedURL();
  if (ipns_url.SchemeIs(ipfs::kIPNSScheme)) {
    rewards_service_->OnLoad(tab_id_, ipns_url);
    return;
  }
#endif

  rewards_service_->OnLoad(tab_id_, validated_url);
}

// TODO(zenparsing): Maybe we should use PrimaryPageChanged instead?
void RewardsTabHelper::DidFinishNavigation(content::NavigationHandle* handle) {
  if (!handle->IsInMainFrame() || !handle->HasCommitted() ||
      handle->IsDownload()) {
    return;
  }

  SetPublisherIdForTab(RewardsService::GetPublisherIdFromURL(
      web_contents()->GetLastCommittedURL()));

  MaybeSavePublisherInfo();

  if (rewards_service_) {
    rewards_service_->OnUnload(tab_id_);
  }
}

void RewardsTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  if (!rewards_service_ || !render_frame_host) {
    return;
  }

  switch (resource_load_info.request_destination) {
    case network::mojom::RequestDestination::kAudio:
    case network::mojom::RequestDestination::kTrack:
    case network::mojom::RequestDestination::kVideo:
    case network::mojom::RequestDestination::kEmpty:
    case network::mojom::RequestDestination::kImage:
    case network::mojom::RequestDestination::kScript:
      rewards_service_->OnXHRLoad(tab_id_, GURL(resource_load_info.final_url),
                                  web_contents()->GetLastCommittedURL(),
                                  resource_load_info.referrer);
      break;
    default:
      break;
  }
}

void RewardsTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  if (!rewards_service_) {
    return;
  }

  if (visibility == content::Visibility::HIDDEN) {
    rewards_service_->OnHide(tab_id_);
  } else if (visibility == content::Visibility::OCCLUDED) {
    rewards_service_->OnBackground(tab_id_);
  } else if (visibility == content::Visibility::VISIBLE) {
    rewards_service_->OnShow(tab_id_);
  }
}

void RewardsTabHelper::WebContentsDestroyed() {
  if (rewards_service_) {
    rewards_service_->OnUnload(tab_id_);
  }
}

#if !BUILDFLAG(IS_ANDROID)
void RewardsTabHelper::OnBrowserSetLastActive(Browser* browser) {
  if (rewards_service_ && BrowserHasWebContents(browser)) {
    rewards_service_->OnForeground(tab_id_);
  }
}
#endif

#if !BUILDFLAG(IS_ANDROID)
void RewardsTabHelper::OnBrowserNoLongerActive(Browser* browser) {
  if (rewards_service_ && BrowserHasWebContents(browser)) {
    rewards_service_->OnBackground(tab_id_);
  }
}
#endif

#if !BUILDFLAG(IS_ANDROID)
bool RewardsTabHelper::BrowserHasWebContents(Browser* browser) {
  int index = browser->tab_strip_model()->GetIndexOfWebContents(web_contents());
  return index != TabStripModel::kNoTab;
}
#endif

void RewardsTabHelper::OnRewardsInitialized(RewardsService* rewards_service) {
  MaybeSavePublisherInfo();
  if (rewards_service_) {
    rewards_service_->OnLoad(tab_id_, web_contents()->GetLastCommittedURL());
  }
}

void RewardsTabHelper::MaybeSavePublisherInfo() {
  // TODO(zenparsing): This is necessary because the system assumes that the
  // publisher_info table is populated as the user navigates the web. In the
  // extension this was accomplished with the chrome.tabs API in the background
  // script.
  if (rewards_service_) {
    rewards_service_->GetPublisherActivityFromUrl(
        tab_id_.id(), web_contents()->GetLastCommittedURL().spec(), "", "");
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(RewardsTabHelper);

}  // namespace brave_rewards
