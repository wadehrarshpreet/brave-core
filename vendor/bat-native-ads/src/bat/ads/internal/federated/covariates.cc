/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariates.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/federated/covariate.h"
#include "bat/ads/internal/federated/covariates/ad_notification_clicked.h"
#include "bat/ads/internal/federated/covariates/ad_notification_served_at.h"
#include "bat/ads/internal/federated/covariates/average_clickthrough_rate.h"
#include "bat/ads/internal/federated/covariates/last_ad_notification_was_clicked.h"
#include "bat/ads/internal/federated/covariates/number_of_user_activity_events.h"
#include "bat/ads/internal/federated/covariates/time_since_last_user_activity_event.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

Covariates* g_covariates = nullptr;

using UserActivityEventToCovariateTypesMapping =
    base::flat_map<UserActivityEventType,
                   std::pair<brave_federated::mojom::CovariateType,
                             brave_federated::mojom::CovariateType>>;
UserActivityEventToCovariateTypesMapping&
GetUserActivityEventToCovariateTypesMapping() {
  static base::NoDestructor<UserActivityEventToCovariateTypesMapping> mappings(
      {{UserActivityEventType::kBrowserDidBecomeActive,
        {brave_federated::mojom::CovariateType::
             kNumberOfBrowserDidBecomeActiveEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastBrowserDidBecomeActiveEvent}},
       {UserActivityEventType::kBrowserWindowIsActive,
        {brave_federated::mojom::CovariateType::
             kNumberOfBrowserWindowIsActiveEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastBrowserWindowIsActiveEvent}},
       {UserActivityEventType::kBrowserWindowIsInactive,
        {brave_federated::mojom::CovariateType::
             kNumberOfBrowserWindowIsInactiveEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastBrowserWindowIsInactiveEvent}},
       {UserActivityEventType::kClickedBackOrForwardNavigationButtons,
        {brave_federated::mojom::CovariateType::
             kNumberOfClickedBackOrForwardNavigationButtonsEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastClickedBackOrForwardNavigationButtonsEvent}},
       {UserActivityEventType::kClickedLink,
        {brave_federated::mojom::CovariateType::kNumberOfClickedLinkEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastClickedLinkEvent}},
       {UserActivityEventType::kClickedReloadButton,
        {brave_federated::mojom::CovariateType::
             kNumberOfClickedReloadButtonEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastClickedReloadButtonEvent}},
       {UserActivityEventType::kClosedTab,
        {brave_federated::mojom::CovariateType::kNumberOfClosedTabEvents,
         brave_federated::mojom::CovariateType::kTimeSinceLastClosedTabEvent}},
       {UserActivityEventType::kFocusedOnExistingTab,
        {brave_federated::mojom::CovariateType::
             kNumberOfFocusedOnExistingTabEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastFocusedOnExistingTabEvent}},
       {UserActivityEventType::kNewNavigation,
        {brave_federated::mojom::CovariateType::kNumberOfNewNavigationEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastNewNavigationEvent}},
       {UserActivityEventType::kOpenedNewTab,
        {brave_federated::mojom::CovariateType::kNumberOfOpenedNewTabEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastOpenedNewTabEvent}},
       {UserActivityEventType::kPlayedMedia,
        {brave_federated::mojom::CovariateType::kNumberOfPlayedMediaEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastPlayedMediaEvent}},
       {UserActivityEventType::kSubmittedForm,
        {brave_federated::mojom::CovariateType::kNumberOfSubmittedFormEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastSubmittedFormEvent}},
       {UserActivityEventType::kTypedAndSelectedNonUrl,
        {brave_federated::mojom::CovariateType::
             kNumberOfTypedAndSelectedNonUrlEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastTypedAndSelectedNonUrlEvent}},
       {UserActivityEventType::kTypedKeywordOtherThanDefaultSearchProvider,
        {brave_federated::mojom::CovariateType::
             kNumberOfTypedKeywordOtherThanDefaultSearchProviderEvents,
         brave_federated::mojom::CovariateType::
             kTimeSinceLastTypedKeywordOtherThanDefaultSearchProviderEvent}},
       {UserActivityEventType::kTypedUrl,
        {brave_federated::mojom::CovariateType::kNumberOfTypedUrlEvents,
         brave_federated::mojom::CovariateType::kTimeSinceLastTypedUrlEvent}}});
  return *mappings;
}

using AverageClickthroughRateTimeWindows = std::vector<base::TimeDelta>;
AverageClickthroughRateTimeWindows& GetAverageClickthroughRateTimeWindows() {
  static base::NoDestructor<std::vector<base::TimeDelta>> time_windows(
      {base::Days(1), base::Days(7), base::Days(28)});
  return *time_windows;
}

}  // namespace

Covariates::Covariates() {
  DCHECK_EQ(g_covariates, nullptr);
  g_covariates = this;

  SetCovariate(std::make_unique<LastAdNotificationWasClicked>());

  for (const auto& user_activity_event_to_covariate_types_mapping :
       GetUserActivityEventToCovariateTypesMapping()) {
    const UserActivityEventType event_type =
        user_activity_event_to_covariate_types_mapping.first;

    const brave_federated::mojom::CovariateType
        number_of_events_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.first;
    SetCovariate(std::make_unique<NumberOfUserActivityEvents>(
        event_type, number_of_events_covariate_type));

    const brave_federated::mojom::CovariateType
        time_since_last_event_covariate_type =
            user_activity_event_to_covariate_types_mapping.second.second;
    SetCovariate(std::make_unique<TimeSinceLastUserActivityEvent>(
        event_type, time_since_last_event_covariate_type));
  }

  for (const auto& average_clickthrough_rate_time_window :
       GetAverageClickthroughRateTimeWindows()) {
    SetCovariate(std::make_unique<AverageClickthroughRate>(
        average_clickthrough_rate_time_window));
  }
}

Covariates::~Covariates() {
  DCHECK(g_covariates);
  g_covariates = nullptr;
}

// static
Covariates* Covariates::Get() {
  DCHECK(g_covariates);
  return g_covariates;
}

// static
bool Covariates::HasInstance() {
  return g_covariates;
}

void Covariates::SetCovariate(std::unique_ptr<Covariate> entry) {
  DCHECK(entry);
  brave_federated::mojom::CovariateType key = entry->GetCovariateType();
  covariates_[key] = std::move(entry);
}

brave_federated::mojom::TrainingInstancePtr Covariates::GetCovariates() const {
  brave_federated::mojom::TrainingInstancePtr training_instance =
      brave_federated::mojom::TrainingInstance::New();
  for (const auto& item : covariates_) {
    const Covariate* covariate = item.second.get();
    DCHECK(covariate);

    brave_federated::mojom::CovariatePtr federated_covariate =
        brave_federated::mojom::Covariate::New();
    federated_covariate->data_type = covariate->GetDataType();
    federated_covariate->covariate_type = covariate->GetCovariateType();
    federated_covariate->value = covariate->GetValue();

    training_instance->covariates.push_back(std::move(federated_covariate));
  }

  return training_instance;
}

void Covariates::SetAdNotificationServedAt(const base::Time time) {
  auto ad_notification_served_at = std::make_unique<AdNotificationServedAt>();
  ad_notification_served_at->SetTime(time);
  SetCovariate(std::move(ad_notification_served_at));
}

void Covariates::SetAdNotificationClicked(bool clicked) {
  auto ad_notification_clicked = std::make_unique<AdNotificationClicked>();
  ad_notification_clicked->SetClicked(clicked);
  SetCovariate(std::move(ad_notification_clicked));
}

void Covariates::AddCovariatesToDataStore() {
  brave_federated::mojom::TrainingInstancePtr training_instance =
      GetCovariates();
  AdsClientHelper::Get()->AddCovariatesToDataStore(
      std::move(training_instance));
}

}  // namespace ads
