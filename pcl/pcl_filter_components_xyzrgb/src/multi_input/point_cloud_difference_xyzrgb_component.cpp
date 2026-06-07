// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/multi_input/point_cloud_difference_filter.hpp"
#include "pcl_filter_components/ros/common/binary_cloud_filter_component.hpp"

namespace pcl_filter_components_xyzrgb
{

using Filter =
  pcl_filter_components::filters::multi_input::PointCloudDifferenceFilter<pcl::PointXYZRGB>;
using PointCloudDifferenceXYZRGBComponent =
  pcl_filter_components::ros::common::BinaryCloudFilterComponent<pcl::PointXYZRGB, Filter>;

}  // namespace pcl_filter_components_xyzrgb

namespace pcl_filter_components::ros::common
{

template class BinaryCloudFilterComponent<
  pcl::PointXYZRGB,
  pcl_filter_components::filters::multi_input::PointCloudDifferenceFilter<pcl::PointXYZRGB>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgb::PointCloudDifferenceXYZRGBComponent)
