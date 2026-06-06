// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/downsampling/random_sample_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(RandomSampleFilterXYZI, UsesRequestedSampleSize)
{
  pcl_filter_components::filters::downsampling::RandomSampleFilter<pcl_filter_tests::Point> filter;
  filter.configure({3, false});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::sampleCloud(), output);

  EXPECT_EQ(output.size(), 3U);
}
