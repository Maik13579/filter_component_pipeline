// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/surface/moving_least_squares_filter.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"

namespace pcl_filter_components_xyzrgb
{

using Filter =
  pcl_filter_components::filters::surface::MovingLeastSquaresFilter<pcl::PointXYZRGB>;
using MovingLeastSquaresXYZRGBComponent =
  pcl_filter_components::ros::common::SingleCloudFilterComponent<pcl::PointXYZRGB, Filter>;

}  // namespace pcl_filter_components_xyzrgb

namespace pcl_filter_components::ros::common
{

template class SingleCloudFilterComponent<
  pcl::PointXYZRGB,
  pcl_filter_components::filters::surface::MovingLeastSquaresFilter<pcl::PointXYZRGB>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzrgb::MovingLeastSquaresXYZRGBComponent)
