// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__RADIUS_OUTLIER_REMOVAL_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__RADIUS_OUTLIER_REMOVAL_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::outlier_removal
{
template <typename PointT>
class RadiusOutlierRemovalFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double radius;
    int min_neighbors;
    bool invert;
  };
  static const char * nodeName()
  {
    return "radius_outlier_removal_filter";
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
  pcl::RadiusOutlierRemoval<PointT> make() const
  {
    pcl::RadiusOutlierRemoval<PointT> f;
    f.setRadiusSearch(params_.radius);
    f.setMinNeighborsInRadius(std::max(params_.min_neighbors, 0));
    f.setNegative(params_.invert);
    return f;
  }
  Params params_{0.25, 2, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__RADIUS_OUTLIER_REMOVAL_FILTER_HPP_
