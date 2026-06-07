// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__GRID_MINIMUM_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__GRID_MINIMUM_FILTER_HPP_

#include <pcl/filters/grid_minimum.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::downsampling
{
template <typename PointT>
class GridMinimumFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double resolution;
  };
  static const char * nodeName()
  {
    return "grid_minimum_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::GridMinimum<PointT> f(static_cast<float>(params_.resolution));
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  Params params_{0.05};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__DOWNSAMPLING__GRID_MINIMUM_FILTER_HPP_
