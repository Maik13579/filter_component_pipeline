// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGB_RANGE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGB_RANGE_FILTER_HPP_

#include "pcl_filter_components/filters/color/color_threshold_filter.hpp"

namespace pcl_filter_components::filters::color
{

template <typename PointT>
class RGBRangeFilter : public ColorThresholdFilter<PointT>
{
public:
  static const char * nodeName()
  {
    return "rgb_range_filter";
  }
};

}  // namespace pcl_filter_components::filters::color

#endif  // PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGB_RANGE_FILTER_HPP_
