// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__RANDOM_SAMPLE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__RANDOM_SAMPLE_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/filters/random_sample.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::downsampling
{
template <typename PointT>
class RandomSampleFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    int sample_size;
    bool invert;
  };
  static const char * nodeName()
  {
    return "random_sample_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::RandomSample<PointT> f;
    f.setSample(std::max(params_.sample_size, 0));
    f.setSeed(1U);
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
    pcl::RandomSample<PointT> f;
    f.setSample(std::max(params_.sample_size, 0));
    f.setSeed(1U);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
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
  Params params_{1000, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__RANDOM_SAMPLE_FILTER_HPP_
