// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/surface/median_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(MedianFilterXYZI, RunsOnOrganizedCloud)
{
  pcl_filter_components::filters::surface::MedianFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({3, 1.0});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::organizedCloud(), output);

  EXPECT_EQ(output.size(), pcl_filter_components_tests::organizedCloud().size());
}
