// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/intensity/intensity_range_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(IntensityRangeFilterXYZI, FiltersByIntensity)
{
  pcl_filter_components::filters::intensity::IntensityRangeFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({1.0, 2.0, false});

  pcl_filter_components_tests::Indices indices;
  filter.filterIndices(pcl_filter_components_tests::sampleCloud(), indices);

  EXPECT_EQ(indices, (pcl_filter_components_tests::Indices{3, 4}));
}
