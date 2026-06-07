// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__VOXEL_GRID_COVARIANCE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__VOXEL_GRID_COVARIANCE_FILTER_HPP_

#include <algorithm>
#include <pcl/filters/voxel_grid_covariance.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class VoxelGridCovarianceFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    float leaf_size_x;
    float leaf_size_y;
    float leaf_size_z;
    int min_points_per_voxel;
  };
  static const char * nodeName()
  {
    return "voxel_grid_covariance_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::VoxelGridCovariance<PointT> f;
    f.setLeafSize(params_.leaf_size_x, params_.leaf_size_y, params_.leaf_size_z);
    f.setMinPointPerVoxel(std::max(params_.min_points_per_voxel, 3));
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  Params params_{0.05F, 0.05F, 0.05F, 3};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__VOXEL_GRID_COVARIANCE_FILTER_HPP_
