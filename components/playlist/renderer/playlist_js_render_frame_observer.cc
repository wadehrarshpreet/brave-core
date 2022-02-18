/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_js_render_frame_observer.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "brave/components/brave_shields/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace playlist {

namespace {

const char kSecurityOrigin[] = "chrome://playlist";

void EnsureIsolatedWorldInitialized(int world_id) {
  static absl::optional<int> last_used_world_id;
  if (last_used_world_id) {
    // Early return since the isolated world info. is already initialized.
    DCHECK_EQ(*last_used_world_id, world_id)
        << "EnsureIsolatedWorldInitialized should always be called with the "
           "same |world_id|";
    return;
  }

  last_used_world_id = world_id;

  // Set an empty CSP so that the main world's CSP is not used in the isolated
  // world.
  constexpr char kContentSecurityPolicy[] = "";

  blink::WebIsolatedWorldInfo info;
  info.security_origin =
      blink::WebSecurityOrigin::Create(GURL(kSecurityOrigin));
  info.content_security_policy =
      blink::WebString::FromUTF8(kContentSecurityPolicy);
  blink::SetIsolatedWorldInfo(world_id, info);
}

}  // namespace

PlaylistJsRenderFrameObserver::PlaylistJsRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistJsRenderFrameObserver>(
          render_frame),
      isolated_world_id_(isolated_world_id),
      native_javascript_handle_(
          new PlaylistJSHandler(render_frame, isolated_world_id)),
      ready_(new base::OneShotEvent()) {}

PlaylistJsRenderFrameObserver::~PlaylistJsRenderFrameObserver() {}

void PlaylistJsRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    absl::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void PlaylistJsRenderFrameObserver::ReadyToCommitNavigation(
    blink::WebDocumentLoader* document_loader) {
  ready_.reset(new base::OneShotEvent());
  // invalidate weak pointers on navigation so we don't get callbacks from the
  // previous url load
  weak_factory_.InvalidateWeakPtrs();

  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();

  if (!url_.SchemeIsHTTPOrHTTPS())
    return;

  // if (base::FeatureList::IsEnabled(
  //         ::brave_shields::features::kCosmeticFilteringSyncLoad)) {
  //   if (native_javascript_handle_->ProcessURL(url_, absl::nullopt)) {
  //     ready_->Signal();
  //   }
  // } else {
  //   native_javascript_handle_->ProcessURL(
  //       url_, absl::make_optional(base::BindOnce(
  //                 &PlaylistJsRenderFrameObserver::OnProcessURL,
  //                 weak_factory_.GetWeakPtr())));
  // }
}

void PlaylistJsRenderFrameObserver::RunScriptsAtDocumentStart() {
  // if (ready_->is_signaled()) {
  //   ApplyRules();
  // } else {
  //   ready_->Post(
  //       FROM_HERE,
  //       base::BindOnce(&PlaylistJsRenderFrameObserver::ApplyRules,
  //                      weak_factory_.GetWeakPtr()));
  // }
}

// void PlaylistJsRenderFrameObserver::ApplyRules() {
//   native_javascript_handle_->ApplyRules();
// }

// void PlaylistJsRenderFrameObserver::OnProcessURL() {
//   ready_->Signal();
// }

void PlaylistJsRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id != isolated_world_id_)
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void PlaylistJsRenderFrameObserver::DidCreateNewDocument() {
  EnsureIsolatedWorldInitialized(isolated_world_id_);
}

void PlaylistJsRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace playlist
