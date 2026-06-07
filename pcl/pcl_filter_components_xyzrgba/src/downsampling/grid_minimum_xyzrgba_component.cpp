// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/downsampling/grid_minimum_filter.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"

namespace pcl_filter_components_xyzrgba
{

using Filter =
  pcl_filter_components::filters::downsampling::GridMinimumFilter<pcl::PointXYZRGBA>;
using GridMinimumXYZRGBAComponent =
  pcl_filter_components::ros::common::SingleCloudFilterComponent<pcl::PointXYZRGBA, Filter>;

}  // namespace pcl_filter_components_xyzrgba

namespace pcl_filter_components::ros::common
{

template class SingleCloudFilterComponent<
  pcl::PointXYZRGBA,
  pcl_filter_components::filters::downsampling::GridMinimumFilter<pcl::PointXYZRGBA>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgba::GridMinimumXYZRGBAComponent)
