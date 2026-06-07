// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/outlier_removal/conditional_removal_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(ConditionalRemovalFilterXYZI, FiltersByField)
{
  pcl_filter_components::filters::outlier_removal::ConditionalRemovalFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({"z", 1.0, 2.0, false});

  pcl_filter_components_tests::Indices indices;
  filter.filterIndices(pcl_filter_components_tests::sampleCloud(), indices);

  EXPECT_EQ(indices, (pcl_filter_components_tests::Indices{0, 1, 2, 3}));
}
