// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_
#define PCL_FILTER_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_

#include <pcl/PointIndices.h>
#include <pcl/point_types.h>

#include "filter_component_base/ros/filter_chain_component.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_indices_type_adapter.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components_filter_chain::ros
{

template <typename PointT>
using PclCloudAdapter =
  pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;

using PclIndicesAdapter =
  pcl_filter_components_type_adapters::ros::PclIndicesAdapter;

struct RosFilterChainXYZTraits
{
  static const char * nodeName() {return "ros_filter_chain_xyz";}
  static const char * dataType() {return "pcl::PointCloud<pcl::PointXYZ>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * originalInputPort() {return "orig_input";}
};

struct RosFilterChainXYZITraits
{
  static const char * nodeName() {return "ros_filter_chain_xyzi";}
  static const char * dataType() {return "pcl::PointCloud<pcl::PointXYZI>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * originalInputPort() {return "orig_input";}
};

struct RosFilterChainXYZRGBTraits
{
  static const char * nodeName() {return "ros_filter_chain_xyzrgb";}
  static const char * dataType() {return "pcl::PointCloud<pcl::PointXYZRGB>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * originalInputPort() {return "orig_input";}
};

struct RosFilterChainXYZRGBATraits
{
  static const char * nodeName() {return "ros_filter_chain_xyzrgba";}
  static const char * dataType() {return "pcl::PointCloud<pcl::PointXYZRGBA>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * originalInputPort() {return "orig_input";}
};

struct RosFilterChainPointNormalTraits
{
  static const char * nodeName() {return "ros_filter_chain_point_normal";}
  static const char * dataType() {return "pcl::PointCloud<pcl::Normal>";}
  static const char * inputPort() {return "cloud";}
  static const char * outputPort() {return "cloud";}
  static const char * originalInputPort() {return "orig_input";}
};

struct RosFilterChainPointIndicesTraits
{
  static const char * nodeName() {return "ros_filter_chain_point_indices";}
  static const char * dataType() {return "pcl::PointIndices";}
  static const char * inputPort() {return "indices";}
  static const char * outputPort() {return "indices";}
  static const char * originalInputPort() {return "orig_input";}
};

using RosFilterChainXYZComponent =
  filter_component_base::ros::FilterChainComponent<
  PclCloudAdapter<pcl::PointXYZ>,
  RosFilterChainXYZTraits>;

using RosFilterChainXYZIComponent =
  filter_component_base::ros::FilterChainComponent<
  PclCloudAdapter<pcl::PointXYZI>,
  RosFilterChainXYZITraits>;

using RosFilterChainXYZRGBComponent =
  filter_component_base::ros::FilterChainComponent<
  PclCloudAdapter<pcl::PointXYZRGB>,
  RosFilterChainXYZRGBTraits>;

using RosFilterChainXYZRGBAComponent =
  filter_component_base::ros::FilterChainComponent<
  PclCloudAdapter<pcl::PointXYZRGBA>,
  RosFilterChainXYZRGBATraits>;

using RosFilterChainPointNormalComponent =
  filter_component_base::ros::FilterChainComponent<
  PclCloudAdapter<pcl::Normal>,
  RosFilterChainPointNormalTraits>;

using RosFilterChainPointIndicesComponent =
  filter_component_base::ros::FilterChainComponent<
  PclIndicesAdapter,
  RosFilterChainPointIndicesTraits>;

}  // namespace pcl_filter_components_filter_chain::ros

#endif  // PCL_FILTER_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_
