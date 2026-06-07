// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__LOCAL_MAXIMUM_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__LOCAL_MAXIMUM_FILTER_HPP_

#include <vector>
#include <pcl/filters/local_maximum.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class LocalMaximumFilter
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
    return "local_maximum_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  pcl::LocalMaximum<PointT> make() const
  {
    pcl::LocalMaximum<PointT> f;
    f.setRadius(static_cast<float>(params_.radius));
    f.setNegative(params_.invert);
    return f;
  }
  Params params_{1.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__LOCAL_MAXIMUM_FILTER_HPP_
