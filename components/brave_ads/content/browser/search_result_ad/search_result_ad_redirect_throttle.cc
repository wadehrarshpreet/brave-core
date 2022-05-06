/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_redirect_throttle.h"

#include <utility>

#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "components/sessions/content/session_tab_helper.h"
#include "services/network/public/cpp/request_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

SessionID GetTabId(content::WebContents* web_contents) {
  DCHECK(web_contents);

  SessionID tab_id = sessions::SessionTabHelper::IdForTab(web_contents);
  content::RenderFrameHost* opener_rfh = web_contents->GetOpener();
  if (!opener_rfh) {
    opener_rfh = web_contents->GetOriginalOpener();
  }
  if (opener_rfh) {
    content::WebContents* original_web_contents =
        content::WebContents::FromRenderFrameHost(opener_rfh);
    tab_id = sessions::SessionTabHelper::IdForTab(original_web_contents);
  }

  return tab_id;
}

}  // namespace

// static
std::unique_ptr<SearchResultAdRedirectThrottle>
SearchResultAdRedirectThrottle::MaybeCreateThrottleFor(
    SearchResultAdService* search_result_ad_service,
    const network::ResourceRequest& request,
    content::WebContents* web_contents) {
  DCHECK(web_contents);

  if (!search_result_ad_service || !request.request_initiator ||
      !request.has_user_gesture) {
    return nullptr;
  }

  // !request.is_main_frame
  if (request.resource_type !=
      static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
    return nullptr;
  }

  if (!brave_search::IsAllowedHost(request.request_initiator->GetURL())) {
    return nullptr;
  }

  if (!IsSearchResultAdClickedConfirmationUrl(request.url)) {
    return nullptr;
  }

  SessionID tab_id = GetTabId(web_contents);
  if (!tab_id.is_valid()) {
    return nullptr;
  }

  return std::make_unique<SearchResultAdRedirectThrottle>(
      search_result_ad_service, tab_id);
}

SearchResultAdRedirectThrottle::SearchResultAdRedirectThrottle(
    SearchResultAdService* search_result_ad_service,
    SessionID tab_id)
    : search_result_ad_service_(search_result_ad_service), tab_id_(tab_id) {
  DCHECK(search_result_ad_service_);
  DCHECK(tab_id_.is_valid());
}

SearchResultAdRedirectThrottle::~SearchResultAdRedirectThrottle() = default;

void SearchResultAdRedirectThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  DCHECK(request);
  DCHECK(request->request_initiator);
  DCHECK(brave_search::IsAllowedHost(request->request_initiator->GetURL()));

  std::string creative_instance_id =
      GetClickedSearchResultAdCreativeInstanceId(request->url);
  if (creative_instance_id.empty()) {
    return;
  }

  absl::optional<GURL> search_result_ad_target_url =
      search_result_ad_service_->MaybeTriggerSearchResultAdClickedEvent(
          creative_instance_id, tab_id_);
  if (!search_result_ad_target_url) {
    return;
  }

  url::Origin original_origin = url::Origin::Create(request->url);
  url::Origin search_result_ad_target_origin =
      url::Origin::Create(*search_result_ad_target_url);
  request->url = *search_result_ad_target_url;

  if (!original_origin.IsSameOriginWith(search_result_ad_target_origin)) {
    request->site_for_cookies =
        net::SiteForCookies::FromOrigin(search_result_ad_target_origin);
    request->trusted_params = network::ResourceRequest::TrustedParams();
    request->trusted_params->isolation_info = net::IsolationInfo::Create(
        net::IsolationInfo::RequestType::kOther, search_result_ad_target_origin,
        search_result_ad_target_origin, request->site_for_cookies);
  }
}

}  // namespace brave_ads
