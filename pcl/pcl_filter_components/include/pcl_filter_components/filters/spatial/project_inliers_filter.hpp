// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PROJECT_INLIERS_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PROJECT_INLIERS_FILTER_HPP_

#include <pcl/filters/project_inliers.h>
#include <pcl/point_cloud.h>
#include <pcl/sample_consensus/model_types.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::spatial
{
template <typename PointT>
class ProjectInliersFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  struct Params
  {
    double a;
    double b;
    double c;
    double d;
  };
  static const char * nodeName()
  {
    return "project_inliers_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    coefficients->values = {
      static_cast<float>(params_.a), static_cast<float>(params_.b), static_cast<float>(params_.c), static_cast<float>(params_.d)
    };
    pcl::ProjectInliers<PointT> f;
    f.setModelType(pcl::SACMODEL_PLANE);
    f.setModelCoefficients(coefficients);
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  Params params_{0.0, 0.0, 1.0, 0.0};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__PROJECT_INLIERS_FILTER_HPP_
