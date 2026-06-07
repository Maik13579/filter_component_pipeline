// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__COLOR__COLOR_THRESHOLD_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__COLOR__COLOR_THRESHOLD_FILTER_HPP_

#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::color
{
template <typename PointT>
class ColorThresholdFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    int min_r;
    int min_g;
    int min_b;
    int max_r;
    int max_g;
    int max_b;
    bool invert;
  };
  static const char * nodeName()
  {
    return "color_threshold_filter";
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
    const int r = common::colorChannel(p, 16);
    const int g = common::colorChannel(p, 8);
    const int b = common::colorChannel(p, 0);
    const bool inside =
      r >= params_.min_r && r <= params_.max_r &&
      g >= params_.min_g && g <= params_.max_g &&
      b >= params_.min_b && b <= params_.max_b;
    return params_.invert ? !inside : inside;
  }
  Params params_{0, 0, 0, 255, 255, 255, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__COLOR__COLOR_THRESHOLD_FILTER_HPP_
