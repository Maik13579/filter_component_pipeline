// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/ros/crop_box_component.hpp"

namespace pcl_filter_components_xyzrgb
{

using CropBoxXYZRGBComponent = pcl_filter_components::ros::CropBoxComponent<pcl::PointXYZRGB>;

}  // namespace pcl_filter_components_xyzrgb

namespace pcl_filter_components::ros
{

template class CropBoxComponent<pcl::PointXYZRGB>;

}  // namespace pcl_filter_components::ros

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgb::CropBoxXYZRGBComponent)
