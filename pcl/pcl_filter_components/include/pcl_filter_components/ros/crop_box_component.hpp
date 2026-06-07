// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/crop_box_filter.hpp"
#include "filter_component_base/ros/parameter_utils.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

using filter_component_base::ros::declareParameterIfNotDeclared;
using filter_component_base::ros::getParameter;
using filter_component_base::ros::makeFloatingPointRangeParameterDescriptor;
using filter_component_base::ros::makeParameterDescriptor;
using filter_component_base::ros::FilterComponentBase;

template <typename PointT>
class CropBoxComponent
  : public FilterComponentBase
{
public:
  using Filter = filters::CropBoxFilter<PointT>;
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit CropBoxComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "crop_box_filter",
      options,
      inputPorts(),
      outputPorts())
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

  void configure() override
  {
    typename Filter::Params params;
    params.min_x = getParameter<double>(*this, "filter.min_x");
    params.min_y = getParameter<double>(*this, "filter.min_y");
    params.min_z = getParameter<double>(*this, "filter.min_z");
    params.max_x = getParameter<double>(*this, "filter.max_x");
    params.max_y = getParameter<double>(*this, "filter.max_y");
    params.max_z = getParameter<double>(*this, "filter.max_z");
    params.invert = getParameter<bool>(*this, "filter.invert");
    filter_.configure(params);
  }

  void process() override
  {
    auto input = this->template takeInput<CloudAdapter>("cloud");
    if (!input) {
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    filter_.filter(*input, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
    this->template publish<CloudAdapter>("orig_cloud", std::move(input));
  }

  Filter filter_;
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
