// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>

#include "pcl_filter_components/filters/outlier_removal/radius_outlier_removal_filter.hpp"
#include "pcl_filter_components/ros/common/single_cloud_filter_component.hpp"

namespace pcl_filter_components_xyzi
{

using Filter =
  pcl_filter_components::filters::outlier_removal::RadiusOutlierRemovalFilter<pcl::PointXYZI>;
using RadiusOutlierRemovalXYZIComponent =
  pcl_filter_components::ros::common::SingleCloudFilterComponent<pcl::PointXYZI, Filter>;

}  // namespace pcl_filter_components_xyzi

namespace pcl_filter_components::ros::common
{

template class SingleCloudFilterComponent<
  pcl::PointXYZI,
  pcl_filter_components::filters::outlier_removal::RadiusOutlierRemovalFilter<pcl::PointXYZI>>;

}  // namespace pcl_filter_components::ros::common

RCLCPP_COMPONENTS_REGISTER_NODE(pcl_filter_components_xyzi::RadiusOutlierRemovalXYZIComponent)
