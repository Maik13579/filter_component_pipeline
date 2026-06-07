// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PLANE_CLIPPER_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PLANE_CLIPPER_FILTER_HPP_

#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::spatial
{
template <typename PointT>
class PlaneClipperFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double a;
    double b;
    double c;
    double d;
    bool invert;
  };
  static const char * nodeName()
  {
    return "plane_clipper_filter";
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
    const bool positive = params_.a * p.x + params_.b * p.y + params_.c * p.z + params_.d >= 0.0;
    return params_.invert ? !positive : positive;
  }
  Params params_{0.0, 0.0, 1.0, 0.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PLANE_CLIPPER_FILTER_HPP_
