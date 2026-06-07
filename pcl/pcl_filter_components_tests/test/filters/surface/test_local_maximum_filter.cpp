// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/surface/local_maximum_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(LocalMaximumFilterXYZI, Runs)
{
  pcl_filter_components::filters::surface::LocalMaximumFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({0.2, false});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::sampleCloud(), output);

  EXPECT_LE(output.size(), pcl_filter_components_tests::sampleCloud().size());
}
