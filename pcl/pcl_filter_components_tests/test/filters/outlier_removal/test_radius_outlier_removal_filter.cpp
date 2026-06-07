// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/outlier_removal/radius_outlier_removal_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(RadiusOutlierRemovalFilterXYZI, FiltersIsolatedPoints)
{
  pcl_filter_components::filters::outlier_removal::RadiusOutlierRemovalFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({0.05, 1, false});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::sampleCloud(), output);

  EXPECT_EQ(output.size(), 2U);
}
