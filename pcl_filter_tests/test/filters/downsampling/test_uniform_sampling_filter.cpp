// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/downsampling/uniform_sampling_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(UniformSamplingFilterXYZI, SelectsRepresentatives)
{
  pcl_filter_components::filters::downsampling::UniformSamplingFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.1, false});

  pcl_filter_tests::Indices indices;
  filter.filterIndices(pcl_filter_tests::sampleCloud(), indices);

  EXPECT_GT(indices.size(), 0U);
  EXPECT_LT(indices.size(), pcl_filter_tests::sampleCloud().size());
}
