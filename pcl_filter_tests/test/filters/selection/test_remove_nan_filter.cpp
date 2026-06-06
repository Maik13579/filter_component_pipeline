// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <limits>

#include "pcl_filter_components/filters/selection/remove_nan_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(RemoveNaNFilterXYZI, FiltersInvalidCoordinates)
{
  auto cloud = pcl_filter_tests::sampleCloud();
  cloud.push_back(pcl_filter_tests::point(std::numeric_limits<float>::quiet_NaN(), 0.0F, 1.0F));
  pcl_filter_tests::finalizeCloud(cloud);

  pcl_filter_components::filters::selection::RemoveNaNFilter<pcl_filter_tests::Point> filter;
  filter.configure({false});

  pcl_filter_tests::Cloud output;
  filter.filter(cloud, output);

  EXPECT_EQ(output.size(), pcl_filter_tests::sampleCloud().size());
}
