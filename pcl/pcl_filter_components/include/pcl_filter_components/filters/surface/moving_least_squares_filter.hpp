// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MOVING_LEAST_SQUARES_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MOVING_LEAST_SQUARES_FILTER_HPP_

#include <algorithm>
#include <pcl/search/kdtree.h>
#include <pcl/surface/mls.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::surface
{
template <typename PointT>
class MovingLeastSquaresFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double search_radius;
    int polynomial_order;
  };
  static const char * nodeName()
  {
    return "moving_least_squares_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::MovingLeastSquares<PointT, PointT> f;
    typename pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    f.setSearchMethod(tree);
    f.setSearchRadius(params_.search_radius);
    f.setPolynomialOrder(std::max(params_.polynomial_order, 0));
    f.setInputCloud(common::makeConstPtr(input));
    f.process(output);
  }
private:
  Params params_{0.03, 2};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SURFACE__MOVING_LEAST_SQUARES_FILTER_HPP_
