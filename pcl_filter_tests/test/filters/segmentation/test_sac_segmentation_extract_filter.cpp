// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/segmentation/sac_segmentation_extract_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(SACSegmentationExtractFilterXYZI, FindsPlane)
{
  pcl_filter_components::filters::segmentation::SACSegmentationExtractFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.02, 100, true, false});

  pcl_filter_tests::Indices indices;
  filter.filterIndices(pcl_filter_tests::planarCloud(), indices);

  EXPECT_GE(indices.size(), 16U);
}
