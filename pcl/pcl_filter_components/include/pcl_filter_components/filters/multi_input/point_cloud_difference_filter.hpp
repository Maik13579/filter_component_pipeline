// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_DIFFERENCE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_DIFFERENCE_FILTER_HPP_

#include "pcl_filter_components/filters/multi_input/point_cloud_subtract_filter.hpp"
namespace pcl_filter_components::filters::multi_input
{
template <typename PointT>
class PointCloudDifferenceFilter : public PointCloudSubtractFilter<PointT>
{
public:
  static const char * nodeName()
  {
    return "point_cloud_difference_filter";
  }
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_DIFFERENCE_FILTER_HPP_
