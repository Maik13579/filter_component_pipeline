// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/ros/point_cloud_merger_component.hpp"

namespace pcl_filter_components_xyzi
{

using PointCloudConcatenateXYZIComponent = pcl_filter_components::ros::PointCloudMergerComponent<pcl::PointXYZI>;

}  // namespace pcl_filter_components_xyzi

namespace pcl_filter_components::ros
{

template class PointCloudMergerComponent<pcl::PointXYZI>;

}  // namespace pcl_filter_components::ros

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzi::PointCloudConcatenateXYZIComponent)
