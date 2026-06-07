// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_INDICES_TYPE_ADAPTER_HPP_
#define PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_INDICES_TYPE_ADAPTER_HPP_

#include <type_traits>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl_msgs/msg/point_indices.hpp>
#include <pcl/PointIndices.h>
#include <rclcpp/type_adapter.hpp>

namespace rclcpp
{

template <>
struct TypeAdapter<
  pcl::PointIndices,
  pcl_msgs::msg::PointIndices>
{
  using is_specialized = std::true_type;
  using custom_type = pcl::PointIndices;
  using ros_message_type = pcl_msgs::msg::PointIndices;

  static void convert_to_ros_message(
    const custom_type & source,
    ros_message_type & destination)
  {
    pcl_conversions::fromPCL(source, destination);
  }

  static void convert_to_custom(
    const ros_message_type & source,
    custom_type & destination)
  {
    pcl_conversions::toPCL(source, destination);
    pcl_conversions::toPCL(source.header, destination.header);
  }
};

}  // namespace rclcpp

namespace pcl_filter_components_type_adapters::ros
{

using StampedPclIndicesAdapter =
  rclcpp::adapt_type<pcl::PointIndices>::as<pcl_msgs::msg::PointIndices>;

using PclIndicesAdapter = StampedPclIndicesAdapter;

}  // namespace pcl_filter_components_type_adapters::ros

#endif  // PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_INDICES_TYPE_ADAPTER_HPP_
