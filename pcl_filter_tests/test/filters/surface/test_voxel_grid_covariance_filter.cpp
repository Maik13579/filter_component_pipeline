// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/surface/voxel_grid_covariance_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(VoxelGridCovarianceFilterXYZI, Runs)
{
  pcl_filter_components::filters::surface::VoxelGridCovarianceFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.2F, 0.2F, 0.2F, 3});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::planarCloud(), output);

  EXPECT_GT(output.size(), 0U);
  EXPECT_LE(output.size(), pcl_filter_tests::planarCloud().size());
}
