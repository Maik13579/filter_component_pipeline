// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/multi_input/point_cloud_difference_filter.hpp"
#include "pcl_filter_components/ros/common/binary_cloud_filter_component.hpp"

namespace pcl_filter_components_xyz
{

using Filter =
  pcl_filter_components::filters::multi_input::PointCloudDifferenceFilter<pcl::PointXYZ>;
using PointCloudDifferenceXYZComponent =
  pcl_filter_components::ros::common::BinaryCloudFilterComponent<pcl::PointXYZ, Filter>;

}  // namespace pcl_filter_components_xyz

namespace pcl_filter_components::ros::common
{

template class BinaryCloudFilterComponent<
  pcl::PointXYZ,
  pcl_filter_components::filters::multi_input::PointCloudDifferenceFilter<pcl::PointXYZ>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyz::PointCloudDifferenceXYZComponent)
