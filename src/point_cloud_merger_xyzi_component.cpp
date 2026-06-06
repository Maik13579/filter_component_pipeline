// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/ros/point_cloud_merger_component.hpp"

namespace pcl_filter_components
{

using PointCloudMergerXYZIComponent = ros::PointCloudMergerComponent<pcl::PointXYZI>;

template class ros::PointCloudMergerComponent<pcl::PointXYZI>;

}  // namespace pcl_filter_components

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components::PointCloudMergerXYZIComponent)
