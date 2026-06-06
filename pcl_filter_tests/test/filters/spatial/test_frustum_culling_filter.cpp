// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/spatial/frustum_culling_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(FrustumCullingFilterXYZI, Runs)
{
  pcl_filter_components::filters::spatial::FrustumCullingFilter<pcl_filter_tests::Point> filter;
  filter.configure({120.0, 120.0, 0.1, 10.0, false});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);

  EXPECT_GT(output.size(), 0U);
  EXPECT_LE(output.size(), pcl_filter_tests::sampleCloud().size());
}
