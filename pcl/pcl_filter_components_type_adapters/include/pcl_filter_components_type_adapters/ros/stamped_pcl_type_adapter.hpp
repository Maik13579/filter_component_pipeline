// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_TYPE_ADAPTER_HPP_
#define PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_TYPE_ADAPTER_HPP_

#include <type_traits>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <rclcpp/type_adapter.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

namespace rclcpp
{

template <typename PointT>
struct TypeAdapter<
  pcl::PointCloud<PointT>,
  sensor_msgs::msg::PointCloud2>
{
  using is_specialized = std::true_type;
  using custom_type = pcl::PointCloud<PointT>;
  using ros_message_type = sensor_msgs::msg::PointCloud2;

  static void convert_to_ros_message(
    const custom_type & source,
    ros_message_type & destination)
  {
    pcl::toROSMsg(source, destination);
  }

  static void convert_to_custom(
    const ros_message_type & source,
    custom_type & destination)
  {
    pcl::fromROSMsg(source, destination);
    pcl_conversions::toPCL(source.header, destination.header);
  }
};

}  // namespace rclcpp

namespace pcl_filter_components_type_adapters::ros
{

template <typename PointT>
using StampedPclCloudAdapter =
  typename rclcpp::adapt_type<pcl::PointCloud<PointT>>
  ::template as<sensor_msgs::msg::PointCloud2>;

template <typename PointT>
using PclCloudAdapter = StampedPclCloudAdapter<PointT>;

using PclNormalCloudAdapter = PclCloudAdapter<pcl::Normal>;

}  // namespace pcl_filter_components_type_adapters::ros

#endif  // PCL_FILTER_COMPONENTS_TYPE_ADAPTERS__ROS__STAMPED_PCL_TYPE_ADAPTER_HPP_
