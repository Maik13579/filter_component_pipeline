// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SELECTION__KEEP_ORGANIZED_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SELECTION__KEEP_ORGANIZED_FILTER_HPP_

#include <cmath>
#include <limits>
#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/selection/remove_infinite_filter.hpp"
namespace pcl_filter_components::filters::selection
{
template <typename PointT>
class KeepOrganizedFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    bool invert;
  };
  static const char * nodeName()
  {
    return "keep_organized_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    output = input;
    for (auto & p: output)
    {
      const bool ok = std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
      if (params_.invert ? ok : !ok)
      {
        p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();
      }
    }
    output.is_dense = false;
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    RemoveInfiniteFilter<PointT> f;
    f.configure( {
        params_.invert
      }
                 );
    f.filterIndices(input, output);
  }
private:
  Params params_{false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SELECTION__KEEP_ORGANIZED_FILTER_HPP_
