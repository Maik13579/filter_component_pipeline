// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_SUBTRACT_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_SUBTRACT_FILTER_HPP_

#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::multi_input
{
template <typename PointT>
class PointCloudSubtractFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double tolerance;
  };
  static const char * nodeName()
  {
    return "point_cloud_subtract_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    output = input;
  }
  void filter(const Cloud & minuend, const Cloud & subtrahend, Cloud & output) const
  {
    common::copyIf(minuend, output, [this, &subtrahend](const auto & p) {
        for (const auto & q: subtrahend)
        {
          const auto dx = p.x - q.x;
          const auto dy = p.y - q.y;
          const auto dz = p.z - q.z;
          const auto distance_squared = dx * dx + dy * dy + dz * dz;
          if (distance_squared <= params_.tolerance * params_.tolerance)
          {
            return false;
          }
        }
        return true;
      }
                   );
  }
private:
  Params params_{1.0e-4};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__MULTI_INPUT__POINT_CLOUD_SUBTRACT_FILTER_HPP_
