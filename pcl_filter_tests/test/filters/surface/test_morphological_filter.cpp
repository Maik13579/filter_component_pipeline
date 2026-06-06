// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <pcl/filters/morphological_filter.h>

#include "pcl_filter_components/filters/surface/morphological_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(MorphologicalFilterXYZI, Runs)
{
  pcl_filter_components::filters::surface::MorphologicalFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.1, pcl::MORPH_OPEN});

  pcl_filter_tests::Cloud output;
  filter.filter(pcl_filter_tests::organizedCloud(), output);

  EXPECT_LE(output.size(), pcl_filter_tests::organizedCloud().size());
}
