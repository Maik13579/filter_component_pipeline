// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <limits>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include "pcl_filter_components/filters/crop_box_filter.hpp"
#include "pcl_filter_components/filters/downsampling/uniform_sampling_filter.hpp"
#include "pcl_filter_components/filters/intensity/intensity_range_filter.hpp"
#include "pcl_filter_components/filters/multi_input/point_cloud_subtract_filter.hpp"
#include "pcl_filter_components/filters/selection/remove_nan_filter.hpp"
#include "pcl_filter_components/filters/spatial/crop_sphere_filter.hpp"
#include "pcl_filter_components/filters/passthrough_filter.hpp"
#include "pcl_filter_components/filters/voxel_grid_filter.hpp"

namespace
{

pcl::PointXYZI makePoint(float x, float y, float z, float intensity = 1.0F)
{
  pcl::PointXYZI point;
  point.x = x;
  point.y = y;
  point.z = z;
  point.intensity = intensity;
  return point;
}

pcl::PointCloud<pcl::PointXYZI> makeCloud()
{
  pcl::PointCloud<pcl::PointXYZI> cloud;
  cloud.push_back(makePoint(0.0F, 0.0F, 0.0F));
  cloud.push_back(makePoint(0.01F, 0.01F, 0.01F));
  cloud.push_back(makePoint(2.0F, 2.0F, 3.0F));
  cloud.width = static_cast<std::uint32_t>(cloud.size());
  cloud.height = 1;
  cloud.is_dense = true;
  return cloud;
}

TEST(VoxelGridFilter, ReducesPointsInSameLeaf)
{
  pcl_filter_components::filters::VoxelGridFilter<pcl::PointXYZI> filter;
  filter.configure({0.1F, 0.1F, 0.1F, false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(makeCloud(), output);

  EXPECT_EQ(output.size(), 2U);
}

TEST(VoxelGridFilter, OutputsRepresentativeIndices)
{
  pcl_filter_components::filters::VoxelGridFilter<pcl::PointXYZI> filter;
  filter.configure({0.1F, 0.1F, 0.1F, false});

  std::vector<int> output;
  filter.filterIndices(makeCloud(), output);

  EXPECT_EQ(output.size(), 2U);
}

TEST(VoxelGridFilter, InvertsRepresentativeIndices)
{
  pcl_filter_components::filters::VoxelGridFilter<pcl::PointXYZI> filter;
  filter.configure({0.1F, 0.1F, 0.1F, true});

  std::vector<int> output;
  filter.filterIndices(makeCloud(), output);

  EXPECT_EQ(output.size(), 1U);
}

TEST(PassThroughFilter, RemovesPointsOutsideLimits)
{
  pcl_filter_components::filters::PassThroughFilter<pcl::PointXYZI> filter;
  filter.configure({"z", -0.5, 1.0, false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(makeCloud(), output);

  ASSERT_EQ(output.size(), 2U);
  for (const auto & point : output) {
    EXPECT_GE(point.z, -0.5F);
    EXPECT_LE(point.z, 1.0F);
  }
}

TEST(PassThroughFilter, OutputsIndicesAndSupportsInvert)
{
  pcl_filter_components::filters::PassThroughFilter<pcl::PointXYZI> filter;
  filter.configure({"z", -0.5, 1.0, false});

  std::vector<int> output;
  filter.filterIndices(makeCloud(), output);
  EXPECT_EQ(output, (std::vector<int>{0, 1}));

  filter.configure({"z", -0.5, 1.0, true});
  filter.filterIndices(makeCloud(), output);
  EXPECT_EQ(output, (std::vector<int>{2}));
}

TEST(CropBoxFilter, RemovesPointsOutsideBounds)
{
  pcl_filter_components::filters::CropBoxFilter<pcl::PointXYZI> filter;
  filter.configure({-1.0, -1.0, -1.0, 1.0, 1.0, 1.0, false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(makeCloud(), output);

  ASSERT_EQ(output.size(), 2U);
  for (const auto & point : output) {
    EXPECT_GE(point.x, -1.0F);
    EXPECT_LE(point.x, 1.0F);
    EXPECT_GE(point.y, -1.0F);
    EXPECT_LE(point.y, 1.0F);
    EXPECT_GE(point.z, -1.0F);
    EXPECT_LE(point.z, 1.0F);
  }
}

TEST(CropBoxFilter, OutputsIndicesAndSupportsInvert)
{
  pcl_filter_components::filters::CropBoxFilter<pcl::PointXYZI> filter;
  filter.configure({-1.0, -1.0, -1.0, 1.0, 1.0, 1.0, false});

  std::vector<int> output;
  filter.filterIndices(makeCloud(), output);
  EXPECT_EQ(output, (std::vector<int>{0, 1}));

  filter.configure({-1.0, -1.0, -1.0, 1.0, 1.0, 1.0, true});
  filter.filterIndices(makeCloud(), output);
  EXPECT_EQ(output, (std::vector<int>{2}));
}

TEST(DownsamplingFilters, UniformSamplingSelectsRepresentatives)
{
  pcl_filter_components::filters::downsampling::UniformSamplingFilter<pcl::PointXYZI> filter;
  filter.configure({0.1F, false});

  std::vector<int> output;
  filter.filterIndices(makeCloud(), output);

  EXPECT_EQ(output.size(), 2U);
}

TEST(SelectionFilters, RemoveNaNDropsInvalidCoordinates)
{
  auto cloud = makeCloud();
  cloud.push_back(makePoint(std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F));

  pcl_filter_components::filters::selection::RemoveNaNFilter<pcl::PointXYZI> filter;
  filter.configure({false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(cloud, output);

  EXPECT_EQ(output.size(), 3U);
}

TEST(SpatialFilters, CropSphereKeepsPointsInsideRadius)
{
  pcl_filter_components::filters::spatial::CropSphereFilter<pcl::PointXYZI> filter;
  filter.configure({0.0, 0.0, 0.0, 0.5, false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(makeCloud(), output);

  EXPECT_EQ(output.size(), 2U);
}

TEST(IntensityFilters, IntensityRangeFiltersByField)
{
  auto cloud = makeCloud();
  cloud[0].intensity = 0.1F;
  cloud[1].intensity = 0.5F;
  cloud[2].intensity = 2.0F;

  pcl_filter_components::filters::intensity::IntensityRangeFilter<pcl::PointXYZI> filter;
  filter.configure({0.2, 1.0, false});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(cloud, output);

  ASSERT_EQ(output.size(), 1U);
  EXPECT_FLOAT_EQ(output.front().intensity, 0.5F);
}

TEST(MultiInputFilters, PointCloudSubtractRemovesNearbyPoints)
{
  const auto cloud = makeCloud();
  pcl::PointCloud<pcl::PointXYZI> removed;
  removed.push_back(cloud.front());

  pcl_filter_components::filters::multi_input::PointCloudSubtractFilter<pcl::PointXYZI> filter;
  filter.configure({0.001});

  pcl::PointCloud<pcl::PointXYZI> output;
  filter.filter(cloud, removed, output);

  EXPECT_EQ(output.size(), 2U);
}

}  // namespace
