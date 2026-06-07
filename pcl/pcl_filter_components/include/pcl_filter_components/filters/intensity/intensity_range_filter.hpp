// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_RANGE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_RANGE_FILTER_HPP_

#include "pcl_filter_components/filters/intensity/intensity_threshold_filter.hpp"

namespace pcl_filter_components::filters::intensity
{

template <typename PointT>
class IntensityRangeFilter : public IntensityThresholdFilter<PointT>
{
public:
  static const char * nodeName()
  {
    return "intensity_range_filter";
  }
};

}  // namespace pcl_filter_components::filters::intensity

#endif  // PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_RANGE_FILTER_HPP_
