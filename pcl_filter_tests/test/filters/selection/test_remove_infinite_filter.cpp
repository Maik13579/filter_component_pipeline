// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <limits>

#include "pcl_filter_components/filters/selection/remove_infinite_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(RemoveInfiniteFilterXYZI, FiltersInvalidCoordinates)
{
  auto cloud = pcl_filter_tests::sampleCloud();
  cloud.push_back(pcl_filter_tests::point(std::numeric_limits<float>::infinity(), 0.0F, 1.0F));
  pcl_filter_tests::finalizeCloud(cloud);

  pcl_filter_components::filters::selection::RemoveInfiniteFilter<pcl_filter_tests::Point> filter;
  filter.configure({false});

  pcl_filter_tests::Indices indices;
  filter.filterIndices(cloud, indices);

  EXPECT_EQ(indices.size(), pcl_filter_tests::sampleCloud().size());
}
