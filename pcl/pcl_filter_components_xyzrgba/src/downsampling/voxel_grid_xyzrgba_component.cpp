// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/ros/voxel_grid_component.hpp"

namespace pcl_filter_components_xyzrgba
{

using VoxelGridXYZRGBAComponent = pcl_filter_components::ros::VoxelGridComponent<pcl::PointXYZRGBA>;

}  // namespace pcl_filter_components_xyzrgba

namespace pcl_filter_components::ros
{

template class VoxelGridComponent<pcl::PointXYZRGBA>;

}  // namespace pcl_filter_components::ros

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgba::VoxelGridXYZRGBAComponent)
