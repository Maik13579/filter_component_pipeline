// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/passthrough_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(PassThroughFilterXYZI, FiltersByField)
{
  pcl_filter_components::filters::PassThroughFilter<pcl_filter_components_tests::Point> filter;
  filter.configure({"z", 1.0, 2.0, false});

  pcl_filter_components_tests::Cloud output;
  filter.filter(pcl_filter_components_tests::sampleCloud(), output);

  ASSERT_EQ(output.size(), 4U);
  for (const auto & p : output) {
    EXPECT_GE(p.z, 1.0F);
    EXPECT_LE(p.z, 2.0F);
  }
}
