// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/spatial/plane_clipper_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(PlaneClipperFilterXYZI, KeepsPositiveSide)
{
  pcl_filter_components::filters::spatial::PlaneClipperFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.0, 0.0, 1.0, -1.1, false});

  pcl_filter_tests::Indices indices;
  filter.filterIndices(pcl_filter_tests::sampleCloud(), indices);

  EXPECT_EQ(indices, (pcl_filter_tests::Indices{2, 3, 4, 5}));
}
