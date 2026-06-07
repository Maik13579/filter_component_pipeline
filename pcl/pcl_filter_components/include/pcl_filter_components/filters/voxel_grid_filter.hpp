// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__VOXEL_GRID_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__VOXEL_GRID_FILTER_HPP_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <pcl/common/point_tests.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/point_cloud.h>

namespace pcl_filter_components::filters
{

template <typename PointT>
class VoxelGridFilter
{
public:
  using PointType = PointT;
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;

  struct Params
  {
    float leaf_size_x;
    float leaf_size_y;
    float leaf_size_z;
    bool invert;
  };

  void configure(const Params & params)
  {
    params_ = params;
  }

  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::VoxelGrid<PointT> voxel_grid;
    voxel_grid.setLeafSize(params_.leaf_size_x, params_.leaf_size_y, params_.leaf_size_z);
    voxel_grid.setInputCloud(makeConstPtr(input));
    voxel_grid.filter(output);
  }

  void filterIndices(const Cloud & input, Indices & output) const
  {
    output.clear();
    if (
      input.empty() ||
      params_.leaf_size_x <= 0.0F ||
      params_.leaf_size_y <= 0.0F ||
      params_.leaf_size_z <= 0.0F)
    {
      return;
    }

    std::unordered_map<VoxelKey, int, VoxelKeyHash> selected;
    selected.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
      const auto & point = input[i];
      if (!pcl::isFinite(point)) {
        continue;
      }
      const VoxelKey key{
        static_cast<std::int64_t>(std::floor(point.x / params_.leaf_size_x)),
        static_cast<std::int64_t>(std::floor(point.y / params_.leaf_size_y)),
        static_cast<std::int64_t>(std::floor(point.z / params_.leaf_size_z))};
      selected.emplace(key, static_cast<int>(i));
    }

    output.reserve(params_.invert ? input.size() - selected.size() : selected.size());
    if (params_.invert) {
      std::vector<bool> is_selected(input.size(), false);
      for (const auto & entry : selected) {
        is_selected[static_cast<std::size_t>(entry.second)] = true;
      }
      for (std::size_t i = 0; i < input.size(); ++i) {
        if (!is_selected[i]) {
          output.push_back(static_cast<int>(i));
        }
      }
    } else {
      for (const auto & entry : selected) {
        output.push_back(entry.second);
      }
      std::sort(output.begin(), output.end());
    }
  }

private:
  struct VoxelKey
  {
    std::int64_t x;
    std::int64_t y;
    std::int64_t z;

    bool operator==(const VoxelKey & other) const
    {
      return x == other.x && y == other.y && z == other.z;
    }
  };

  struct VoxelKeyHash
  {
    std::size_t operator()(const VoxelKey & key) const
    {
      std::size_t seed = std::hash<std::int64_t>{}(key.x);
      seed ^= std::hash<std::int64_t>{}(key.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= std::hash<std::int64_t>{}(key.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }
  };

  static typename Cloud::ConstPtr makeConstPtr(const Cloud & cloud)
  {
    return typename Cloud::ConstPtr(&cloud, [](const Cloud *) {});
  }

  Params params_{0.05F, 0.05F, 0.05F, false};
};

}  // namespace pcl_filter_components::filters

#endif  // PCL_FILTER_COMPONENTS__FILTERS__VOXEL_GRID_FILTER_HPP_
