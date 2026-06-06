// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/voxel_grid_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(VoxelGridFilterXYZI, FiltersCloudAndIndices)
{
  pcl_filter_components::filters::VoxelGridFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.1F, 0.1F, 0.1F, false});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);
  EXPECT_LT(output.size(), pcl_filter_tests::sampleCloud().size());

  pcl_filter_tests::Indices indices;
  filter.filterIndices(pcl_filter_tests::sampleCloud(), indices);
  EXPECT_EQ(indices.size(), output.size());
}
