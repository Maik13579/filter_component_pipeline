// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/selection/remove_infinite_filter.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"

namespace pcl_filter_components_xyz
{

using Filter =
  pcl_filter_components::filters::selection::RemoveInfiniteFilter<pcl::PointXYZ>;
using RemoveInfiniteXYZComponent =
  pcl_filter_components::ros::common::SingleCloudFilterComponent<pcl::PointXYZ, Filter>;

}  // namespace pcl_filter_components_xyz

namespace pcl_filter_components::ros::common
{

template class SingleCloudFilterComponent<
  pcl::PointXYZ,
  pcl_filter_components::filters::selection::RemoveInfiniteFilter<pcl::PointXYZ>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyz::RemoveInfiniteXYZComponent)
