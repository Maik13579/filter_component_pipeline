// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__UNIFORM_SAMPLING_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__UNIFORM_SAMPLING_FILTER_HPP_

#include <algorithm>
#include <limits>
#include <vector>
#include <pcl/filters/uniform_sampling.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::downsampling
{
template <typename PointT>
class UniformSamplingFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double radius;
    bool invert;
  };
  static const char * nodeName()
  {
    return "uniform_sampling_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::UniformSampling<PointT> f;
    f.setRadiusSearch(params_.radius);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
    if (params_.invert)
    {
      Indices idx;
      filterIndices(input, idx);
      pcl::copyPointCloud(input, idx, output);
    }
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    Cloud sampled;
    pcl::UniformSampling<PointT> f;
    f.setRadiusSearch(params_.radius);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(sampled);
    output.clear();
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
  Params params_{0.05, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__UNIFORM_SAMPLING_FILTER_HPP_
