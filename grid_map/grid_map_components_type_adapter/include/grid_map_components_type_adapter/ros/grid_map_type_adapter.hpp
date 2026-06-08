// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef GRID_MAP_COMPONENTS_TYPE_ADAPTER__ROS__GRID_MAP_TYPE_ADAPTER_HPP_
#define GRID_MAP_COMPONENTS_TYPE_ADAPTER__ROS__GRID_MAP_TYPE_ADAPTER_HPP_

#include <stdexcept>
#include <type_traits>

#include <grid_map_core/grid_map_core.hpp>
#include <grid_map_msgs/msg/grid_map.hpp>
#include <grid_map_ros/GridMapRosConverter.hpp>
#include <rclcpp/type_adapter.hpp>

namespace rclcpp
{

template <>
struct TypeAdapter<
  grid_map::GridMap,
  grid_map_msgs::msg::GridMap>
{
  using is_specialized = std::true_type;
  using custom_type = grid_map::GridMap;
  using ros_message_type = grid_map_msgs::msg::GridMap;

  static void convert_to_ros_message(
    const custom_type & source,
    ros_message_type & destination)
  {
    auto message = grid_map::GridMapRosConverter::toMessage(source);
    if (!message) {
      throw std::runtime_error("Failed to convert grid_map::GridMap to grid_map_msgs::msg::GridMap");
    }
    destination = *message;
  }

  static void convert_to_custom(
    const ros_message_type & source,
    custom_type & destination)
  {
    if (!grid_map::GridMapRosConverter::fromMessage(source, destination)) {
      throw std::runtime_error("Failed to convert grid_map_msgs::msg::GridMap to grid_map::GridMap");
    }
  }
};

}  // namespace rclcpp

namespace grid_map_components_type_adapter::ros
{

using GridMapAdapter =
  rclcpp::adapt_type<grid_map::GridMap>::as<grid_map_msgs::msg::GridMap>;

}  // namespace grid_map_components_type_adapter::ros

#endif  // GRID_MAP_COMPONENTS_TYPE_ADAPTER__ROS__GRID_MAP_TYPE_ADAPTER_HPP_
