// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_base/ros/parameter_utils.hpp"
#include "pcl_filter_base/ros/pcl_filter_component_base.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"

namespace pcl_filter_components::ros::common
{

template <typename PointT, typename FilterT>
class BinaryCloudFilterComponent : public PclFilterComponentBase<PointT, FilterT>
{
 public:
  using Base = PclFilterComponentBase<PointT, FilterT>;
  using CloudAdapter = typename Base::CloudAdapter;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = typename Base::StampedCloud;

  explicit BinaryCloudFilterComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : Base(FilterT::nodeName(), options, inputPorts(), outputPorts())
  {
    detail::declareParams<FilterT>(*this);
  }
 protected:
  static std::array<PortDescriptor, 2> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>("input_1", "/points/input_a", "First input point cloud topic."),
      Base::template inputPort<CloudAdapter>("input_2", "/points/input_b", "Second input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 3> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>("cloud", "/points/output", "Filtered point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_1",
        "/points/original_a",
        "Original first input point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_2",
        "/points/original_b",
        "Original second input point cloud topic."),
    }};
  }

  void configureFilter() override
  {
    this->filter_.configure(detail::readParams<FilterT>(*this));
  }

  void processInputs() override
  {
    auto first = this->template takeInput<CloudAdapter>("input_1");
    auto second = this->template takeInput<CloudAdapter>("input_2");
    if (!first || !second)
    {
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = first->header;
    this->filter_.filter(*first, *second, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
    this->publishCloud("orig_input_1", std::move(first));
    this->publishCloud("orig_input_2", std::move(second));
  }
};

}  // namespace pcl_filter_components::ros::common

#endif  // PCL_FILTER_COMPONENTS__ROS__COMMON__BINARY_CLOUD_FILTER_COMPONENT_HPP_
