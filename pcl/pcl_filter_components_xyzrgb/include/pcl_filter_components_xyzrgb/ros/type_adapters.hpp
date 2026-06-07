// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_XYZRGB__ROS__TYPE_ADAPTERS_HPP_
#define PCL_FILTER_XYZRGB__ROS__TYPE_ADAPTERS_HPP_

#include <pcl/point_types.h>

#include "pcl_filter_components_type_adapters/ros/stamped_pcl_indices_type_adapter.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components_xyzrgb::ros
{

using PclCloudAdapterPointXYZRGB =
  pcl_filter_components_type_adapters::ros::PclCloudAdapter<pcl::PointXYZRGB>;
using PclIndicesAdapter = pcl_filter_components_type_adapters::ros::PclIndicesAdapter;

}  // namespace pcl_filter_components_xyzrgb::ros

#endif  // PCL_FILTER_XYZRGB__ROS__TYPE_ADAPTERS_HPP_
