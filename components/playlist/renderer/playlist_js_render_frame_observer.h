/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/playlist/renderer/playlist_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace playlist {

// CosmeticFiltersJsRenderFrame observer waits for a page to be loaded and then
// adds the Javascript worker object.
class PlaylistJsRenderFrameObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<
          PlaylistJsRenderFrameObserver> {
 public:
  PlaylistJsRenderFrameObserver(content::RenderFrame* render_frame,
                                       const int32_t isolated_world_id);
  ~PlaylistJsRenderFrameObserver() override;

  PlaylistJsRenderFrameObserver(
      const PlaylistJsRenderFrameObserver&) = delete;
  PlaylistJsRenderFrameObserver& operator=(
      const PlaylistJsRenderFrameObserver&) = delete;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      absl::optional<blink::WebNavigationType> navigation_type) override;
  void ReadyToCommitNavigation(
      blink::WebDocumentLoader* document_loader) override;

  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;
  void DidCreateNewDocument() override;

  void RunScriptsAtDocumentStart();

 private:
  // void OnProcessURL();
  // void ApplyRules();

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // The isolated world that the cosmetic filters object should be written to.
  int32_t isolated_world_id_;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<PlaylistJSHandler> native_javascript_handle_;

  GURL url_;

  std::unique_ptr<base::OneShotEvent> ready_;

  base::WeakPtrFactory<PlaylistJsRenderFrameObserver> weak_factory_{
      this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_RENDER_FRAME_OBSERVER_H_
