// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_

#include <array>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/passthrough_filter.hpp"
#include "pcl_filter_base/ros/parameter_utils.hpp"
#include "pcl_filter_base/ros/pcl_filter_component_base.hpp"

namespace pcl_filter_components::ros
{

using pcl_filter_base::ros::declareParameterIfNotDeclared;
using pcl_filter_base::ros::getParameter;
using pcl_filter_base::ros::makeFloatingPointRangeParameterDescriptor;
using pcl_filter_base::ros::makeParameterDescriptor;
using pcl_filter_base::ros::PclFilterComponentBase;

template <typename PointT>
class PassThroughComponent
  : public PclFilterComponentBase<PointT, filters::PassThroughFilter<PointT>>
{
public:
  using Base = PclFilterComponentBase<PointT, filters::PassThroughFilter<PointT>>;
  using CloudAdapter = typename Base::CloudAdapter;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = typename Base::StampedCloud;

  explicit PassThroughComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "passthrough_filter",
      options,
      inputPorts(),
      outputPorts())
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
  static std::array<PortDescriptor, 1> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>(
        "cloud",
        "Input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 2> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "Filtered point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_cloud",
        "Original input point cloud topic."),
    }};
  }

  void configureFilter() override
  {
    typename filters::PassThroughFilter<PointT>::Params params;
    params.field_name = getParameter<std::string>(*this, "filter.field_name");
    params.min_value = getParameter<double>(*this, "filter.min_value");
    params.max_value = getParameter<double>(*this, "filter.max_value");
    params.invert = getParameter<bool>(*this, "filter.invert");
    this->filter_.configure(params);
  }

  void process() override
  {
    auto input = this->template takeInput<CloudAdapter>("cloud");
    if (!input) {
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    this->filter_.filter(*input, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
    this->template publish<CloudAdapter>("orig_cloud", std::move(input));
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
