/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/renderer_context_menu/brave_render_view_context_menu_views.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/common/channel_info.h"
#include "ui/base/resource/resource_bundle.h"

#include "third_party/blink/public/mojom/context_menu/context_menu.mojom.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/components/ipfs/ipfs_utils.h"
#endif

using blink::mojom::ContextMenuDataMediaType;

BraveRenderViewContextMenuViews::BraveRenderViewContextMenuViews(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
    : RenderViewContextMenuViews(render_frame_host, params)
#if BUILDFLAG(IPFS_ENABLED)
      ,
      ipfs_submenu_model_(this)
#endif
{
}

BraveRenderViewContextMenuViews::~BraveRenderViewContextMenuViews() = default;

// static
RenderViewContextMenuViews* BraveRenderViewContextMenuViews::Create(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {
  return new BraveRenderViewContextMenuViews(render_frame_host, params);
}

void BraveRenderViewContextMenuViews::Show() {
  RemoveAdjacentSeparators();
  RenderViewContextMenuViews::Show();
}

bool BraveRenderViewContextMenuViews::IsCommandIdEnabled(int id) const {
  switch (id) {
#if BUILDFLAG(IPFS_ENABLED)
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      return IsIPFSCommandIdEnabled(id);
#endif
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
#if BUILDFLAG(ENABLE_TOR)
      if (brave::IsTorDisabledForProfile(GetProfile()))
        return false;

      return params_.link_url.is_valid() &&
             IsURLAllowedInIncognito(params_.link_url, browser_context_) &&
             !GetProfile()->IsTor();
#else
      return false;
#endif
    default:
      return RenderViewContextMenu_Chromium::IsCommandIdEnabled(id);
  }
}

#if BUILDFLAG(IPFS_ENABLED)
bool BraveRenderViewContextMenuViews::IsIPFSCommandIdEnabled(
    int command) const {
  if (!ipfs::IsIpfsMenuEnabled(GetProfile()->GetPrefs()))
    return false;
  switch (command) {
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS:
      return true;
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
      return source_web_contents_->GetURL().SchemeIsHTTPOrHTTPS() &&
             source_web_contents_->IsSavable();
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
      return params_.has_image_contents;
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
      return content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO);
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
      return content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO);
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
      return !params_.link_url.is_empty();
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      return !params_.selection_text.empty() &&
             params_.media_type == ContextMenuDataMediaType::kNone;
    default:
      NOTREACHED();
  }
  return false;
}
#endif

#if BUILDFLAG(IPFS_ENABLED)
void BraveRenderViewContextMenuViews::SeIpfsIconAt(int index) {
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  const auto& ipfs_logo = *bundle.GetImageSkiaNamed(IDR_BRAVE_IPFS_LOGO);
  ui::ImageModel model = ui::ImageModel::FromImageSkia(ipfs_logo);
  menu_model_.SetIcon(index, model);
}

void BraveRenderViewContextMenuViews::BuildIPFSMenu() {
  if (!ipfs::IsIpfsMenuEnabled(GetProfile()->GetPrefs()))
    return;
  int index =
      menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_INSPECTELEMENT);
  if (index == -1)
    return;
  if (!params_.selection_text.empty() &&
      params_.media_type == ContextMenuDataMediaType::kNone) {
    menu_model_.InsertSeparatorAt(index,
                                  ui::MenuSeparatorType::NORMAL_SEPARATOR);

    menu_model_.InsertItemWithStringIdAt(
        index, IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_SELECTED_TEXT);
    SeIpfsIconAt(index);
    return;
  }

  auto page_url = source_web_contents_->GetURL();
  if (page_url.SchemeIsHTTPOrHTTPS() &&
      !ipfs::IsAPIGateway(page_url.GetOrigin(), chrome::GetChannel())) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_PAGE);
  }
  if (params_.has_image_contents) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_IMAGE);
  }
  if (content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_VIDEO)) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_VIDEO);
  }
  if (content_type_->SupportsGroup(
          ContextMenuContentType::ITEM_GROUP_MEDIA_AUDIO)) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_AUDIO);
  }
  if (!params_.link_url.is_empty()) {
    ipfs_submenu_model_.AddItemWithStringId(
        IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS,
        IDS_CONTENT_CONTEXT_IMPORT_IPFS_LINK);
  }
  if (!ipfs_submenu_model_.GetItemCount())
    return;
  menu_model_.InsertSeparatorAt(index, ui::MenuSeparatorType::NORMAL_SEPARATOR);
  menu_model_.InsertSubMenuWithStringIdAt(
      index, IDC_CONTENT_CONTEXT_IMPORT_IPFS, IDS_CONTENT_CONTEXT_IMPORT_IPFS,
      &ipfs_submenu_model_);
  SeIpfsIconAt(index);
}
#endif

void BraveRenderViewContextMenuViews::InitMenu() {
  RenderViewContextMenuViews::InitMenu();

#if BUILDFLAG(ENABLE_TOR) || !BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  int index = -1;
#endif
#if BUILDFLAG(ENABLE_TOR)
  // Add Open Link with Tor
  if (!TorProfileServiceFactory::IsTorDisabled() &&
      !params_.link_url.is_empty()) {
    const Browser* browser = GetBrowser();
    const bool is_app = browser && browser->is_type_app();

    index = menu_model_.GetIndexOfCommandId(
        IDC_CONTENT_CONTEXT_OPENLINKOFFTHERECORD);
    DCHECK_NE(index, -1);

    menu_model_.InsertItemWithStringIdAt(
        index + 1, IDC_CONTENT_CONTEXT_OPENLINKTOR,
        is_app ? IDS_CONTENT_CONTEXT_OPENLINKTOR_INAPP
               : IDS_CONTENT_CONTEXT_OPENLINKTOR);
  }
#endif

#if BUILDFLAG(IPFS_ENABLED)
  BuildIPFSMenu();
#endif

  // Only show the translate item when go-translate is enabled.
#if !BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  index = menu_model_.GetIndexOfCommandId(IDC_CONTENT_CONTEXT_TRANSLATE);
  if (index != -1)
    menu_model_.RemoveItemAt(index);
#endif
}
