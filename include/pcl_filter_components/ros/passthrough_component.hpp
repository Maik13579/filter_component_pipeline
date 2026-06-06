// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_

#include <string>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/passthrough_filter.hpp"
#include "pcl_filter_components/ros/optional_indices_output_component.hpp"
#include "pcl_filter_components/ros/parameter_utils.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT>
class PassThroughComponent
  : public OptionalIndicesOutputComponent<PointT, filters::PassThroughFilter<PointT>>
{
public:
  explicit PassThroughComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : OptionalIndicesOutputComponent<PointT, filters::PassThroughFilter<PointT>>(
      "passthrough_filter",
      options)
  {
    declareParameterIfNotDeclared(
      *this,
      "filter.field_name",
      std::string{"z"},
      makeParameterDescriptor(
        "Point field used for pass-through filtering.",
        "Use a scalar field present in the input point type, such as x, y, z, or intensity."));
    declareParameterIfNotDeclared(
      *this,
      "filter.min_value",
      -1.0,
      makeFloatingPointRangeParameterDescriptor(
        "Inclusive lower limit for the selected point field.",
        -1.0e9,
        1.0e9));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_value",
      2.0,
      makeFloatingPointRangeParameterDescriptor(
        "Inclusive upper limit for the selected point field.",
        -1.0e9,
        1.0e9));
    declareParameterIfNotDeclared(
      *this,
      "filter.invert",
      false,
      makeParameterDescriptor("Keep points outside the pass-through range when enabled."));
  }

protected:
  void configureFilter() override
  {
    typename filters::PassThroughFilter<PointT>::Params params;
    params.field_name = getParameter<std::string>(*this, "filter.field_name");
    params.min_value = getParameter<double>(*this, "filter.min_value");
    params.max_value = getParameter<double>(*this, "filter.max_value");
    params.invert = getParameter<bool>(*this, "filter.invert");
    this->filter_.configure(params);
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
