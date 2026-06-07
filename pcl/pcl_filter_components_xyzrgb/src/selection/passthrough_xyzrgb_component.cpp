// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/ros/passthrough_component.hpp"

namespace pcl_filter_components_xyzrgb
{

using PassThroughXYZRGBComponent = pcl_filter_components::ros::PassThroughComponent<pcl::PointXYZRGB>;

}  // namespace pcl_filter_components_xyzrgb

namespace pcl_filter_components::ros
{

template class PassThroughComponent<pcl::PointXYZRGB>;

}  // namespace pcl_filter_components::ros

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgb::PassThroughXYZRGBComponent)
