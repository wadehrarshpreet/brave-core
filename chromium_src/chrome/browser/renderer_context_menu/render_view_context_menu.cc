// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

#include "brave/browser/autocomplete/brave_autocomplete_scheme_classifier.h"
#include "brave/browser/renderer_context_menu/brave_spelling_options_submenu_observer.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/grit/brave_theme_resources.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/common/channel_info.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "ui/base/models/menu_separator_types.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/paint_vector_icon.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif
// Our .h file creates a masquerade for RenderViewContextMenu.  Switch
// back to the Chromium one for the Chromium implementation.
#undef RenderViewContextMenu
#define RenderViewContextMenu RenderViewContextMenu_Chromium

namespace {

GURL GetSelectionNavigationURL(Profile* profile, const std::u16string& text) {
  AutocompleteMatch match;
  AutocompleteClassifier classifier(
      std::make_unique<AutocompleteController>(
          std::make_unique<ChromeAutocompleteProviderClient>(profile),
          AutocompleteClassifier::DefaultOmniboxProviders()),
      std::make_unique<BraveAutocompleteSchemeClassifier>(profile));
  classifier.Classify(text, false, false,
                      metrics::OmniboxEventProto::INVALID_SPEC, &match, NULL);
  classifier.Shutdown();
  return match.destination_url;
}

base::OnceCallback<void(BraveRenderViewContextMenu*)>*
BraveGetMenuShownCallback() {
  static base::NoDestructor<
      base::OnceCallback<void(BraveRenderViewContextMenu*)>>
      callback;
  return callback.get();
}

}  // namespace

void RenderViewContextMenu::RegisterMenuShownCallbackForTesting(
    base::OnceCallback<void(BraveRenderViewContextMenu*)> cb) {
  *BraveGetMenuShownCallback() = std::move(cb);
}

#define BRAVE_APPEND_SEARCH_PROVIDER \
  if (GetProfile()->IsOffTheRecord()) { \
    selection_navigation_url_ = \
        GetSelectionNavigationURL(GetProfile(), params_.selection_text); \
    if (!selection_navigation_url_.is_valid()) \
      return; \
  }

// Use our subclass to initialize SpellingOptionsSubMenuObserver.
#define SpellingOptionsSubMenuObserver BraveSpellingOptionsSubMenuObserver
#define RegisterMenuShownCallbackForTesting \
  RegisterMenuShownCallbackForTesting_unused

#include "../../../../../chrome/browser/renderer_context_menu/render_view_context_menu.cc"

#undef SpellingOptionsSubMenuObserver
#undef RegisterMenuShownCallbackForTesting

// Make it clear which class we mean here.
#undef RenderViewContextMenu
#undef BRAVE_APPEND_SEARCH_PROVIDER

BraveRenderViewContextMenu::BraveRenderViewContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params)
    : RenderViewContextMenu_Chromium(render_frame_host, params)
{
}

#if BUILDFLAG(IPFS_ENABLED)
void BraveRenderViewContextMenu::ExecuteIPFSCommand(int id, int event_flags) {
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(source_web_contents_);
  if (!helper)
    return;
  auto* controller = helper->GetImportController();
  if (!controller)
    return;
  switch (id) {
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
      helper->ImportCurrentPageToIpfs();
      break;
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
      controller->ImportLinkToIpfs(params_.src_url);
      break;
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
      controller->ImportLinkToIpfs(params_.link_url);
      break;
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      controller->ImportTextToIpfs(base::UTF16ToUTF8(params_.selection_text));
      break;
  }
}
#endif

void BraveRenderViewContextMenu::ExecuteCommand(int id, int event_flags) {
  switch (id) {
#if BUILDFLAG(IPFS_ENABLED)
    case IDC_CONTENT_CONTEXT_IMPORT_IPFS_PAGE:
    case IDC_CONTENT_CONTEXT_IMPORT_IMAGE_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_VIDEO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_AUDIO_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_LINK_IPFS:
    case IDC_CONTENT_CONTEXT_IMPORT_SELECTED_TEXT_IPFS:
      ExecuteIPFSCommand(id, event_flags);
      break;
#endif
    case IDC_CONTENT_CONTEXT_OPENLINKTOR:
      TorProfileManager::SwitchToTorProfile(
          GetProfile(),
          base::BindRepeating(
              OnProfileCreated, params_.link_url,
              content::Referrer(
                  GURL(), network::mojom::ReferrerPolicy::kStrictOrigin)));
      break;
    default:
      RenderViewContextMenu_Chromium::ExecuteCommand(id, event_flags);
  }
}

void BraveRenderViewContextMenu::AddSpellCheckServiceItem(bool is_checked) {
  // Call our implementation, not the one in the base class.
  // Assumption:
  // Use of spelling service is disabled in Brave profile preferences.
  DCHECK(!GetProfile()->GetPrefs()->GetBoolean(
      spellcheck::prefs::kSpellCheckUseSpellingService));
  AddSpellCheckServiceItem(&menu_model_, is_checked);
}

// static
void BraveRenderViewContextMenu::AddSpellCheckServiceItem(
    ui::SimpleMenuModel* menu,
    bool is_checked) {
  // Suppress adding "Spellcheck->Ask Brave for suggestions" item.
}
