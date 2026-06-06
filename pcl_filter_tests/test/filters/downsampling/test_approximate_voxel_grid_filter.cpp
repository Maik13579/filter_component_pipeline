// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/downsampling/approximate_voxel_grid_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(ApproximateVoxelGridFilterXYZI, Runs)
{
  pcl_filter_components::filters::downsampling::ApproximateVoxelGridFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.1F, 0.1F, 0.1F, false});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);

  EXPECT_GT(output.size(), 0U);
  EXPECT_LE(output.size(), pcl_filter_tests::sampleCloud().size());
}
