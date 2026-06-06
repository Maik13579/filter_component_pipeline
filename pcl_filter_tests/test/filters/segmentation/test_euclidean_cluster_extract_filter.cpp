// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "pcl_filter_components/filters/segmentation/euclidean_cluster_extract_filter.hpp"
#include "xyzi_filter_test_utils.hpp"

TEST(EuclideanClusterExtractFilterXYZI, FindsLargestCluster)
{
  auto cloud = pcl_filter_tests::sampleCloud();
  cloud.push_back(pcl_filter_tests::point(5.01F, 5.0F, 5.0F));
  pcl_filter_tests::finalizeCloud(cloud);

  pcl_filter_components::filters::segmentation::EuclideanClusterExtractFilter<pcl_filter_tests::Point> filter;
  filter.configure({0.05, 1, 100, 0, false});

  pcl_filter_tests::Indices indices;
  filter.filterIndices(cloud, indices);

  EXPECT_EQ(indices.size(), 2U);
}
