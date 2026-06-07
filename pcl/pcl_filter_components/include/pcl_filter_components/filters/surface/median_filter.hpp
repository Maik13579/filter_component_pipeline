// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MEDIAN_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MEDIAN_FILTER_HPP_

#include <algorithm>
#include <pcl/filters/median_filter.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class MedianFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    int window_size;
    double max_allowed_movement;
  };
  static const char * nodeName()
  {
    return "median_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::MedianFilter<PointT> f;
    f.setWindowSize(std::max(params_.window_size, 1));
    f.setMaxAllowedMovement(static_cast<float>(params_.max_allowed_movement));
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  Params params_{5, std::numeric_limits<float>::max()};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MEDIAN_FILTER_HPP_
