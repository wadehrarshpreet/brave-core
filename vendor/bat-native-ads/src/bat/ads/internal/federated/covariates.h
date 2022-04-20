/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATES_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class Covariate;

// |Covariates| collect training data (i.e. a set of machine learning features)
// for services such as federated learning, tuning and evaluation. They are
// called "covariates" to differentiate them from Chromium/griffin features.
// Covariates can be of different data types as defined in
// |brave_federated::mojom::Covariate|. All covariates are only session based at
// the moment, i.e no measurements are persisted across sessions.
class Covariates final {
 public:
  Covariates();
  Covariates(const Covariates&) = delete;
  Covariates& operator=(const Covariates&) = delete;
  ~Covariates();

  static Covariates* Get();

  static bool HasInstance();

  void SetCovariate(std::unique_ptr<Covariate> covariate);
  brave_federated::mojom::TrainingInstancePtr GetCovariates() const;

  void SetAdNotificationServedAt(const base::Time time);
  void SetAdNotificationClicked(bool clicked);

  void AddCovariatesToDataStore();

 private:
  base::flat_map<brave_federated::mojom::CovariateType,
                 std::unique_ptr<Covariate>>
      covariates_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATES_H_
