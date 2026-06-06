// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/crop_box_filter.hpp"
#include "pcl_filter_components/ros/optional_indices_output_component.hpp"
#include "pcl_filter_components/ros/parameter_utils.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT>
class CropBoxComponent
  : public OptionalIndicesOutputComponent<PointT, filters::CropBoxFilter<PointT>>
{
public:
  explicit CropBoxComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : OptionalIndicesOutputComponent<PointT, filters::CropBoxFilter<PointT>>(
      "crop_box_filter",
      options)
  {
    declareParameterIfNotDeclared(
      *this,
      "filter.min_x",
      -10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum x bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.min_y",
      -10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum y bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.min_z",
      -2.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum z bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_x",
      10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum x bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_y",
      10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum y bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_z",
      3.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum z bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.invert",
      false,
      makeParameterDescriptor("Keep points outside the crop box when enabled."));
  }

protected:
  void configureFilter() override
  {
    typename filters::CropBoxFilter<PointT>::Params params;
    params.min_x = getParameter<double>(*this, "filter.min_x");
    params.min_y = getParameter<double>(*this, "filter.min_y");
    params.min_z = getParameter<double>(*this, "filter.min_z");
    params.max_x = getParameter<double>(*this, "filter.max_x");
    params.max_y = getParameter<double>(*this, "filter.max_y");
    params.max_z = getParameter<double>(*this, "filter.max_z");
    params.invert = getParameter<bool>(*this, "filter.invert");
    this->filter_.configure(params);
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
