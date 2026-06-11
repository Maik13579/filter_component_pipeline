// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros::common
{

using filter_component_base::ros::PortDescriptor;

template <typename PointT, typename FilterT>
class BinaryCloudFilterComponent : public FilterComponentBase
{
 public:
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit BinaryCloudFilterComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : Base(FilterT::nodeName(), options, inputPorts(), outputPorts())
  {
    detail::declareParams<FilterT>(*this);
  }
 protected:
  static std::array<PortDescriptor, 2> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>("input_1", "First input point cloud topic."),
      Base::template inputPort<CloudAdapter>("input_2", "Second input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 3> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>("cloud", "Filtered point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_1",
        "Original first input point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_2",
        "Original second input point cloud topic."),
    }};
  }

  void configure() override
  {
    filter_.configure(detail::readParams<FilterT>(*this));
  }

  void process() override
  {
    auto first = this->template takeInput<CloudAdapter>("input_1");
    auto second = this->template takeInput<CloudAdapter>("input_2");
    if (!first || !second)
    {
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = first->header;
    filter_.filter(*first, *second, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
    this->template publish<CloudAdapter>("orig_input_1", std::move(first));
    this->template publish<CloudAdapter>("orig_input_2", std::move(second));
  }

  FilterT filter_;
};

}  // namespace pcl_filter_components::ros::common

#endif  // PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_
