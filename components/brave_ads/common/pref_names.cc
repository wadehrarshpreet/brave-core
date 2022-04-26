/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"

namespace brave_ads {

namespace prefs {

// Stores whether Brave ads is enabled or disabled
const char kEnabled[] = "brave.brave_ads.enabled";

// Stores whether ads were disabled at least once
const char kAdsWereDisabled[] = "brave.brave_ads.were_disabled";

// Indicates whether we have any initial state of the ads status metric, besides
// "No Wallet".
const char kHasAdsP3AState[] = "brave.brave_ads.has_p3a_state";

// Prefix for preference names pertaining to p2a weekly metrics
const char kP2AStoragePrefNamePrefix[] = "brave.weekly_storage.";

// Stores whether we should show the My First Ad notification
const char kShouldShowMyFirstAdNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

// Stores the supported country codes current schema version number
const char kSupportedCountryCodesLastSchemaVersion[] =
    "brave.brave_ads.supported_regions_last_schema_version_number";
const char kSupportedCountryCodesSchemaVersion[] =
    "brave.brave_ads.supported_regions_schema_version_number";
const int kSupportedCountryCodesSchemaVersionNumber = 9;

// Stores the last screen position of custom ad notifications and whether to
// fallback from native to custom ad notifications if native notifications are
// disabled
const char kAdNotificationLastScreenPositionX[] =
    "brave.brave_ads.ad_notification.last_screen_position_x";
const char kAdNotificationLastScreenPositionY[] =
    "brave.brave_ads.ad_notification.last_screen_position_y";
const char kAdNotificationDidFallbackToCustom[] =
    "brave.brave_ads.ad_notification.did_fallback_to_custom";

// Stores whether Brave ads should allow conversion tracking
const char kShouldAllowConversionTracking[] =
    "brave.brave_ads.should_allow_ad_conversion_tracking";

// Stores the maximum amount of ads per hour
const char kAdsPerHour[] = "brave.brave_ads.ads_per_hour";

// Stores the idle time threshold before checking if an ad can be served
const char kIdleTimeThreshold[] = "brave.brave_ads.idle_threshold";

// Stores whether Brave ads should allow subdivision ad targeting
const char kShouldAllowAdsSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected ads subdivision targeting code
const char kAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected ads subdivision targeting code
const char kAutoDetectedAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

// Stores catalog id
const char kCatalogId[] = "brave.brave_ads.catalog_id";

// Stores catalog version
const char kCatalogVersion[] = "brave.brave_ads.catalog_version";

// Stores catalog ping
const char kCatalogPing[] = "brave.brave_ads.catalog_ping";

// Stores catalog last updated
const char kCatalogLastUpdated[] = "brave.brave_ads.catalog_last_updated";

// Stores issuer ping
const char kIssuerPing[] = "brave.brave_ads.issuer_ping";

// Stores epsilon greedy bandit arms
const char kEpsilonGreedyBanditArms[] =
    "brave.brave_ads.epsilon_greedy_bandit_arms";

// Stores epsilon greedy bandit eligible segments
const char kEpsilonGreedyBanditEligibleSegments[] =
    "brave.brave_ads.epsilon_greedy_bandit_eligible_segments";

// Rewards
const char kNextTokenRedemptionAt[] =
    "brave.brave_ads.rewards.next_time_redemption_at";

// Stores migration status
const char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
const char kHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";

// Stores the preferences version number
const char kVersion[] = "brave.brave_ads.prefs.version";

const int kCurrentVersionNumber = 11;

}  // namespace prefs

}  // namespace brave_ads
