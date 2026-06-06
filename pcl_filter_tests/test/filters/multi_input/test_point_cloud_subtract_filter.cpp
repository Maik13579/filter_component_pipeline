// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/multi_input/point_cloud_subtract_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(PointCloudSubtractFilterXYZI, RemovesNearbyPoints)
{
  const auto cloud = pcl_filter_tests::sampleCloud();
  pcl_filter_tests::Cloud removed;
  removed.push_back(cloud.front());
  pcl_filter_tests::finalizeCloud(removed);

  pcl_filter_components::filters::multi_input::PointCloudSubtractFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.001});

  pcl_filter_tests::Cloud output;
  filter.filter(cloud, removed, output);

  EXPECT_EQ(output.size(), cloud.size() - 1U);
}
