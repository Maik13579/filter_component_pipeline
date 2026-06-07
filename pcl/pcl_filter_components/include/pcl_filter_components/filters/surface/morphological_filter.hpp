// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MORPHOLOGICAL_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MORPHOLOGICAL_FILTER_HPP_

#include <pcl/filters/morphological_filter.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class MorphologicalFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double resolution;
    int operation;
  };
  static const char * nodeName()
  {
    return "morphological_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::applyMorphologicalOperator<PointT>(common::makeConstPtr(input), static_cast<float>(params_.resolution), params_.operation, output);
  }
private:
  Params params_{0.1, pcl::MORPH_OPEN};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MORPHOLOGICAL_FILTER_HPP_
