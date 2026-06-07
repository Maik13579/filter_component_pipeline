// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SELECTION__REMOVE_INFINITE_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SELECTION__REMOVE_INFINITE_FILTER_HPP_

#include <cmath>
#include <vector>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::selection
{
template <typename PointT>
class RemoveInfiniteFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    bool invert;
  };
  static const char * nodeName()
  {
    return "remove_infinite_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    common::copyIf(input, output, [this](const auto & p) {
        return accepts(p);
      }
                   );
  }

  void filterIndices(const Cloud & input, Indices & output) const
  {
    common::indicesIf(input, output, [this](const auto & p) {
        return accepts(p);
      }
                      );
  }

private:
  bool accepts(const PointT & p) const
  {
    const bool finite = std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
    return params_.invert ? !finite : finite;
  }

  Params params_{false};
};

}  // namespace pcl_filter_components::filters::selection

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SELECTION__REMOVE_INFINITE_FILTER_HPP_
