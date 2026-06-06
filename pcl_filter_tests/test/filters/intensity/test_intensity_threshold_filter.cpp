// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/intensity/intensity_threshold_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(IntensityThresholdFilterXYZI, FiltersByIntensity)
{
  pcl_filter_components::filters::intensity::IntensityThresholdFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.2, 1.0, false});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);

  ASSERT_EQ(output.size(), 2U);
  EXPECT_FLOAT_EQ(output.front().intensity, 0.5F);
}
