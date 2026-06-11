// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/crop_box_filter.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

using filter_component_base::ros::FilterComponentBase;
using filter_component_base::ros::PortDescriptor;

template <typename PointT>
class CropBoxComponent
  : public FilterComponentBase
{
public:
  using Filter = filters::CropBoxFilter<PointT>;
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit CropBoxComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "crop_box_filter",
      options,
      inputPorts(),
      outputPorts())
  {
    this->declareParameter(
      "filter.min_x",
      -10.0,
      "Minimum x bound of the crop box in meters.");
    this->declareParameter(
      "filter.min_y",
      -10.0,
      "Minimum y bound of the crop box in meters.");
    this->declareParameter(
      "filter.min_z",
      -2.0,
      "Minimum z bound of the crop box in meters.");
    this->declareParameter(
      "filter.max_x",
      10.0,
      "Maximum x bound of the crop box in meters.");
    this->declareParameter(
      "filter.max_y",
      10.0,
      "Maximum y bound of the crop box in meters.");
    this->declareParameter(
      "filter.max_z",
      3.0,
      "Maximum z bound of the crop box in meters.");
    this->declareParameter(
      "filter.invert",
      false,
      "Keep points outside the crop box when enabled.");
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
    params.min_x = this->template getParameter<double>("filter.min_x");
    params.min_y = this->template getParameter<double>("filter.min_y");
    params.min_z = this->template getParameter<double>("filter.min_z");
    params.max_x = this->template getParameter<double>("filter.max_x");
    params.max_y = this->template getParameter<double>("filter.max_y");
    params.max_z = this->template getParameter<double>("filter.max_z");
    params.invert = this->template getParameter<bool>("filter.invert");
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
