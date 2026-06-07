// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/segmentation/plane_model_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(PlaneModelFilterXYZI, FiltersByDistance)
{
  pcl_filter_components::filters::segmentation::PlaneModelFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({0.0, 0.0, 1.0, 0.0, 0.05, false});

  pcl_filter_components_tests::Indices indices;
  filter.filterIndices(pcl_filter_components_tests::planarCloud(), indices);

  EXPECT_EQ(indices.size(), 16U);
}
