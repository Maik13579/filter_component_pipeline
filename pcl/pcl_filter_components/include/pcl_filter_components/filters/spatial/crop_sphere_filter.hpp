// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__CROP_SPHERE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__CROP_SPHERE_FILTER_HPP_

#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::spatial
{
template <typename PointT>
class CropSphereFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double center_x;
    double center_y;
    double center_z;
    double radius;
    bool invert;
  };
  static const char * nodeName()
  {
    return "crop_sphere_filter";
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
    const auto dx = p.x - params_.center_x, dy = p.y - params_.center_y, dz = p.z - params_.center_z;
    const bool inside = dx * dx + dy * dy + dz * dz <= params_.radius * params_.radius;
    return params_.invert ? !inside : inside;
  }
  Params params_{0.0, 0.0, 0.0, 10.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__CROP_SPHERE_FILTER_HPP_
