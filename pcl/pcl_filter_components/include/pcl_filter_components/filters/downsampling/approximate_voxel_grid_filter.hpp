// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__APPROXIMATE_VOXEL_GRID_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__APPROXIMATE_VOXEL_GRID_FILTER_HPP_

#include <algorithm>
#include <limits>
#include <vector>
#include <pcl/filters/approximate_voxel_grid.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::downsampling
{
template <typename PointT>
class ApproximateVoxelGridFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    float leaf_size_x;
    float leaf_size_y;
    float leaf_size_z;
    bool invert;
  };
  static const char * nodeName()
  {
    return "approximate_voxel_grid_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::ApproximateVoxelGrid<PointT> f;
    f.setLeafSize(params_.leaf_size_x, params_.leaf_size_y, params_.leaf_size_z);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    output.clear();
    Cloud sampled;
    filter(input, sampled);
    for (const auto & s: sampled)
    {
      auto best = input.size();
      auto best_dist = std::numeric_limits<float>::max();
      for (std::size_t i = 0; i < input.size(); ++i)
      {
        const auto dx = s.x - input[i].x, dy = s.y - input[i].y, dz = s.z - input[i].z;
        const auto dist = dx * dx + dy * dy + dz * dz;
        if (dist < best_dist)
        {
          best = i;
          best_dist = dist;
        }
      }
      if (best < input.size())
      {
        output.push_back(static_cast<int>(best));
      }
    }
    std::sort(output.begin(), output.end());
    output.erase(std::unique(output.begin(), output.end()), output.end());
    if (params_.invert)
    {
      std::vector<bool> selected(input.size(), false);
      for (int i: output)
      {
        selected[static_cast<std::size_t>(i)] = true;
      }
      output.clear();
      for (std::size_t i = 0; i < input.size(); ++i)
      {
        if (!selected[i])
        {
          output.push_back(static_cast<int>(i));
        }
      }
    }
  }
private:
  Params params_{0.05F, 0.05F, 0.05F, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__APPROXIMATE_VOXEL_GRID_FILTER_HPP_
