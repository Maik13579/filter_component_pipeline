// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/selection/extract_indices_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(ExtractIndicesFilterXYZI, UsesConfiguredRange)
{
  pcl_filter_components::filters::selection::ExtractIndicesFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({1, 3, false, false});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::sampleCloud(), output);

  ASSERT_EQ(output.size(), 3U);
  EXPECT_FLOAT_EQ(output.front().x, 0.01F);
}
