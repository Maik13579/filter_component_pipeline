// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "pcl_filter_components/filters/selection/keep_organized_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(KeepOrganizedFilterXYZI, PreservesLayout)
{
  auto cloud = pcl_filter_tests::organizedCloud();
  cloud[3].z = std::numeric_limits<float>::infinity();
  cloud.is_dense = false;

  pcl_filter_components::filters::selection::KeepOrganizedFilter<pcl_filter_tests::Point> filter;
  filter.configure({false});

  pcl_filter_tests::Cloud output;
  filter.filter(cloud, output);

  EXPECT_EQ(output.width, cloud.width);
  EXPECT_EQ(output.height, cloud.height);
  EXPECT_FALSE(output.is_dense);
  EXPECT_TRUE(std::isnan(output[3].z));
}
