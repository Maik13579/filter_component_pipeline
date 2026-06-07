// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_TESTS__FILTERS__XYZI_FILTER_TEST_UTILS_HPP_
#define PCL_FILTER_TESTS__FILTERS__XYZI_FILTER_TEST_UTILS_HPP_

#include <cstdint>
#include <vector>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

namespace pcl_filter_components_tests
{

using Point = pcl::PointXYZI;
using Cloud = pcl::PointCloud<Point>;
using Indices = std::vector<int>;

inline Point point(float x, float y, float z, float intensity = 1.0F)
{
  Point p;
  p.x = x;
  p.y = y;
  p.z = z;
  p.intensity = intensity;
  return p;
}

inline void finalizeCloud(Cloud & cloud, std::uint32_t height = 1U)
{
  cloud.height = height;
  cloud.width = static_cast<std::uint32_t>(cloud.size() / height);
  cloud.is_dense = true;
}

inline Cloud sampleCloud()
{
  Cloud cloud;
  cloud.push_back(point(0.0F, 0.0F, 1.0F, 0.1F));
  cloud.push_back(point(0.01F, 0.01F, 1.01F, 0.5F));
  cloud.push_back(point(0.2F, 0.0F, 1.2F, 0.9F));
  cloud.push_back(point(1.0F, 1.0F, 2.0F, 1.5F));
  cloud.push_back(point(2.0F, 2.0F, 3.0F, 2.0F));
  cloud.push_back(point(5.0F, 5.0F, 5.0F, 3.0F));
  finalizeCloud(cloud);
  return cloud;
}

inline Cloud planarCloud()
{
  Cloud cloud;
  for (int x = 0; x < 4; ++x) {
    for (int y = 0; y < 4; ++y) {
      cloud.push_back(point(static_cast<float>(x) * 0.05F, static_cast<float>(y) * 0.05F, 0.0F));
    }
  }
  cloud.push_back(point(0.5F, 0.5F, 0.5F));
  cloud.push_back(point(0.6F, 0.6F, 0.6F));
  finalizeCloud(cloud);
  return cloud;
}

inline Cloud organizedCloud()
{
  Cloud cloud;
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      cloud.push_back(point(static_cast<float>(x) * 0.05F, static_cast<float>(y) * 0.05F, 1.0F));
    }
  }
  finalizeCloud(cloud, 4U);
  return cloud;
}

}  // namespace pcl_filter_components_tests

#endif  // PCL_FILTER_TESTS__FILTERS__XYZI_FILTER_TEST_UTILS_HPP_
