// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGBA_ALPHA_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGBA_ALPHA_FILTER_HPP_

#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::color
{
template <typename PointT>
class RGBAAlphaFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    int min_alpha;
    int max_alpha;
    bool invert;
  };
  static const char * nodeName()
  {
    return "rgba_alpha_filter";
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
    const int a = common::colorChannel(p, 24);
    const bool inside = a >= params_.min_alpha&&a <= params_.max_alpha;
    return params_.invert ? !inside : inside;
  }
  Params params_{1, 255, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__COLOR__RGBA_ALPHA_FILTER_HPP_
