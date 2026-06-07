// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/surface/moving_least_squares_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(MovingLeastSquaresFilterXYZI, Runs)
{
  pcl_filter_components::filters::surface::MovingLeastSquaresFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({0.2, 1});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::planarCloud(), output);

  EXPECT_GT(output.size(), 0U);
  EXPECT_LE(output.size(), pcl_filter_components_tests::planarCloud().size());
}
