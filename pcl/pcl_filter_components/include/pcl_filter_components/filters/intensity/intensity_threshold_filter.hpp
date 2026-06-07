// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_THRESHOLD_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_THRESHOLD_FILTER_HPP_

#include <limits>
#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::intensity
{
template <typename PointT>
class IntensityThresholdFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double min_intensity;
    double max_intensity;
    bool invert;
  };
  static const char * nodeName()
  {
    return "intensity_threshold_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    common::copyIf(input, output, [this](const auto & p) {
        return accept(p);
      }
                   );
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    common::indicesIf(input, output, [this](const auto & p) {
        return accept(p);
      }
                      );
  }
private:
  bool accept(const PointT & p) const
  {
    const auto value = common::intensity(p);
    const bool inside = value >= params_.min_intensity&&value <= params_.max_intensity;
    return params_.invert ? !inside : inside;
  }
  Params params_{0.0, std::numeric_limits<double>::max(), false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__INTENSITY__INTENSITY_THRESHOLD_FILTER_HPP_
