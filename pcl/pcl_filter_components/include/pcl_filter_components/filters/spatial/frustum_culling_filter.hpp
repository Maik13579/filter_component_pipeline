// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__FRUSTUM_CULLING_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__FRUSTUM_CULLING_FILTER_HPP_

#include <Eigen/Core>
#include <vector>
#include <pcl/filters/frustum_culling.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::spatial
{
template <typename PointT>
class FrustumCullingFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double horizontal_fov;
    double vertical_fov;
    double near_plane;
    double far_plane;
    bool invert;
  };
  static const char * nodeName()
  {
    return "frustum_culling_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  pcl::FrustumCulling<PointT> make() const
  {
    pcl::FrustumCulling<PointT> f;
    f.setHorizontalFOV(static_cast<float>(params_.horizontal_fov));
    f.setVerticalFOV(static_cast<float>(params_.vertical_fov));
    f.setNearPlaneDistance(static_cast<float>(params_.near_plane));
    f.setFarPlaneDistance(static_cast<float>(params_.far_plane));
    f.setCameraPose(Eigen::Matrix4f::Identity());
    f.setNegative(params_.invert);
    return f;
  }
  Params params_{90.0, 60.0, 0.0, 100.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SPATIAL__FRUSTUM_CULLING_FILTER_HPP_
