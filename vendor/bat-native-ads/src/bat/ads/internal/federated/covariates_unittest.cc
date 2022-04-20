/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariates.h"

#include "base/time/time.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAdsCovariatesTest*

namespace ads {

class BatAdsCovariatesTest : public UnitTestBase {
 protected:
  BatAdsCovariatesTest() = default;

  ~BatAdsCovariatesTest() override = default;
};

TEST_F(BatAdsCovariatesTest, GetCovariates) {
  // Arrange

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      Covariates::Get()->GetCovariates();

  // Assert
  EXPECT_EQ(32U, training_covariates->covariates.size());
}

TEST_F(BatAdsCovariatesTest, GetCovariatesWithSetters) {
  // Arrange
  Covariates::Get()->SetAdNotificationServedAt(Now());
  Covariates::Get()->SetAdNotificationClicked(true);

  // Act
  brave_federated::mojom::TrainingInstancePtr training_covariates =
      Covariates::Get()->GetCovariates();

  // Assert
  EXPECT_EQ(34U, training_covariates->covariates.size());
}

}  // namespace ads
