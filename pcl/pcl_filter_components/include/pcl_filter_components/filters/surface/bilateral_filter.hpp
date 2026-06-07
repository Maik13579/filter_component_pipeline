// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__BILATERAL_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__BILATERAL_FILTER_HPP_

#include <pcl/filters/bilateral.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class BilateralFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double half_size;
    double stddev;
  };
  static const char * nodeName()
  {
    return "bilateral_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::BilateralFilter<PointT> f;
    f.setHalfSize(params_.half_size);
    f.setStdDev(params_.stddev);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  Params params_{0.05, 0.03};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__BILATERAL_FILTER_HPP_
