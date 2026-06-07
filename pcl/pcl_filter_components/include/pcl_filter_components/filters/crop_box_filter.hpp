// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__CROP_BOX_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__CROP_BOX_FILTER_HPP_

#include <memory>
#include <vector>

#include <Eigen/Core>
#include <pcl/filters/crop_box.h>
#include <pcl/point_cloud.h>

namespace pcl_filter_components::filters
{

template <typename PointT>
class CropBoxFilter
{
public:
  using PointType = PointT;
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;

  struct Params
  {
    double min_x;
    double min_y;
    double min_z;
    double max_x;
    double max_y;
    double max_z;
    bool invert;
  };

  void configure(const Params & params)
  {
    params_ = params;
  }

  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::CropBox<PointT> crop_box;
    crop_box.setMin(Eigen::Vector4f(
      static_cast<float>(params_.min_x),
      static_cast<float>(params_.min_y),
      static_cast<float>(params_.min_z),
      1.0F));
    crop_box.setMax(Eigen::Vector4f(
      static_cast<float>(params_.max_x),
      static_cast<float>(params_.max_y),
      static_cast<float>(params_.max_z),
      1.0F));
    crop_box.setNegative(params_.invert);
    crop_box.setInputCloud(makeConstPtr(input));
    crop_box.filter(output);
  }

  void filterIndices(const Cloud & input, Indices & output) const
  {
    pcl::CropBox<PointT> crop_box;
    crop_box.setMin(Eigen::Vector4f(
      static_cast<float>(params_.min_x),
      static_cast<float>(params_.min_y),
      static_cast<float>(params_.min_z),
      1.0F));
    crop_box.setMax(Eigen::Vector4f(
      static_cast<float>(params_.max_x),
      static_cast<float>(params_.max_y),
      static_cast<float>(params_.max_z),
      1.0F));
    crop_box.setNegative(params_.invert);
    crop_box.setInputCloud(makeConstPtr(input));
    crop_box.filter(output);
  }

private:
  static typename Cloud::ConstPtr makeConstPtr(const Cloud & cloud)
  {
    return typename Cloud::ConstPtr(&cloud, [](const Cloud *) {});
  }

  Params params_{-10.0, -10.0, -2.0, 10.0, 10.0, 3.0, false};
};

}  // namespace pcl_filter_components::filters

#endif  // PCL_FILTER_COMPONENTS__FILTERS__CROP_BOX_FILTER_HPP_
