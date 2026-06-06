// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/spatial/project_inliers_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(ProjectInliersFilterXYZI, ProjectsOntoPlane)
{
  pcl_filter_components::filters::spatial::ProjectInliersFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.0, 0.0, 1.0, 0.0});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);

  ASSERT_FALSE(output.empty());
  for (const auto & p : output) {
    EXPECT_NEAR(p.z, 0.0F, 1.0e-5F);
  }
}
