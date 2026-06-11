// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_

#include <array>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/passthrough_filter.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

using filter_component_base::ros::FilterComponentBase;
using filter_component_base::ros::PortDescriptor;

template <typename PointT>
class PassThroughComponent
  : public FilterComponentBase
{
public:
  using Filter = filters::PassThroughFilter<PointT>;
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit PassThroughComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "passthrough_filter",
      options,
      inputPorts(),
      outputPorts())
  {
    this->declareParameter(
      "filter.field_name",
      std::string{"z"},
      "Point field used for pass-through filtering.");
    this->declareParameter(
      "filter.min_value",
      -1.0,
      "Inclusive lower limit for the selected point field.");
    this->declareParameter(
      "filter.max_value",
      2.0,
      "Inclusive upper limit for the selected point field.");
    this->declareParameter(
      "filter.invert",
      false,
      "Keep points outside the pass-through range when enabled.");
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
    params.field_name = this->template getParameter<std::string>("filter.field_name");
    params.min_value = this->template getParameter<double>("filter.min_value");
    params.max_value = this->template getParameter<double>("filter.max_value");
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

#endif  // PCL_FILTER_COMPONENTS__ROS__PASSTHROUGH_COMPONENT_HPP_
