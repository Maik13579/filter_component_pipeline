// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/type_adapter.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "pcl_filter_components_type_adapters/ros/stamped_pcl_indices_type_adapter.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace
{

using Cloud = pcl::PointCloud<pcl::PointXYZI>;
using Adapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<pcl::PointXYZI>;
using NormalCloud = pcl::PointCloud<pcl::Normal>;
using NormalAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<pcl::Normal>;
using IndicesAdapter = pcl_filter_components_type_adapters::ros::PclIndicesAdapter;

pcl::PointXYZI makePoint(float x, float y, float z, float intensity)
{
  pcl::PointXYZI point;
  point.x = x;
  point.y = y;
  point.z = z;
  point.intensity = intensity;
  return point;
}

std_msgs::msg::Header makeHeader()
{
  std_msgs::msg::Header header;
  header.frame_id = "map";
  header.stamp.sec = 12;
  // pcl::PCLHeader stores time in microseconds, so keep this test on a
  // millisecond boundary and do not assert arbitrary nanosecond precision.
  header.stamp.nanosec = 345000000;
  return header;
}

TEST(PclTypeAdapter, PreservesHeaderAndPointDataRoundTrip)
{
  Cloud original;
  const auto original_header = makeHeader();
  pcl_conversions::toPCL(original_header, original.header);

  const auto point = makePoint(1.0F, 2.0F, 3.0F, 42.0F);
  original.push_back(point);
  original.width = 1;
  original.height = 1;
  original.is_dense = true;

  sensor_msgs::msg::PointCloud2 ros_message;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(original, ros_message);

  Cloud custom_message;
  rclcpp::TypeAdapter<Adapter>::convert_to_custom(ros_message, custom_message);

  sensor_msgs::msg::PointCloud2 round_trip;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(custom_message, round_trip);

  Cloud result;
  rclcpp::TypeAdapter<Adapter>::convert_to_custom(round_trip, result);

  std_msgs::msg::Header result_header;
  pcl_conversions::fromPCL(result.header, result_header);
  EXPECT_EQ(result_header.frame_id, original_header.frame_id);
  EXPECT_EQ(result_header.stamp.sec, original_header.stamp.sec);
  EXPECT_EQ(result_header.stamp.nanosec, original_header.stamp.nanosec);
  ASSERT_EQ(result.size(), 1U);
  EXPECT_FLOAT_EQ(result.front().x, point.x);
  EXPECT_FLOAT_EQ(result.front().y, point.y);
  EXPECT_FLOAT_EQ(result.front().z, point.z);
  EXPECT_FLOAT_EQ(result.front().intensity, point.intensity);
}

TEST(PclTypeAdapter, KeepsRosHeaderUnchangedAfterCustomRoundTrip)
{
  Cloud source;
  const auto header = makeHeader();
  pcl_conversions::toPCL(header, source.header);
  source.push_back(makePoint(5.0F, 6.0F, 7.0F, 8.0F));
  source.width = 1;
  source.height = 1;

  sensor_msgs::msg::PointCloud2 original_ros;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(source, original_ros);

  Cloud custom;
  rclcpp::TypeAdapter<Adapter>::convert_to_custom(original_ros, custom);

  sensor_msgs::msg::PointCloud2 round_trip;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(custom, round_trip);

  EXPECT_EQ(round_trip.header.frame_id, original_ros.header.frame_id);
  EXPECT_EQ(round_trip.header.stamp.sec, original_ros.header.stamp.sec);
  EXPECT_EQ(round_trip.header.stamp.nanosec, original_ros.header.stamp.nanosec);
}

TEST(PclTypeAdapter, IntraProcessUniquePtrPreservesCloudMemoryAddress)
{
  const auto log_dir = std::filesystem::path{"/tmp/pcl_filter_components_type_adapters_test_logs"};
  std::filesystem::create_directories(log_dir);
  setenv("ROS_LOG_DIR", log_dir.c_str(), 1);
  setenv("ROS_AUTOMATIC_DISCOVERY_RANGE", "LOCALHOST", 1);

  int argc = 0;
  char ** argv = nullptr;
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
    "type_adapter_zero_copy_test",
    rclcpp::NodeOptions().use_intra_process_comms(true));

  Cloud * published_message = nullptr;
  const pcl::PointXYZI * published_point_data = nullptr;
  Cloud * received_message = nullptr;
  const pcl::PointXYZI * received_point_data = nullptr;

  auto subscription = node->create_subscription<Adapter>(
    "points",
    rclcpp::QoS(1),
    [&](std::unique_ptr<Cloud> message) {
      received_message = message.get();
      received_point_data = message->points.data();
    });

  auto publisher = node->create_publisher<Adapter>("points", rclcpp::QoS(1));

  auto message = std::make_unique<Cloud>();
  message->push_back(makePoint(1.0F, 2.0F, 3.0F, 4.0F));
  message->width = 1;
  message->height = 1;
  published_message = message.get();
  published_point_data = message->points.data();

  publisher->publish(std::move(message));

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  const auto start = std::chrono::steady_clock::now();
  while (
    received_message == nullptr &&
    std::chrono::steady_clock::now() - start < std::chrono::seconds(2))
  {
    executor.spin_some(std::chrono::milliseconds(10));
  }
  executor.remove_node(node);

  EXPECT_EQ(received_message, published_message);
  EXPECT_EQ(received_point_data, published_point_data);

  subscription.reset();
  publisher.reset();
  node.reset();
  rclcpp::shutdown();
}

TEST(PclTypeAdapter, ConvertsNormalClouds)
{
  NormalCloud normals;
  const auto header = makeHeader();
  pcl_conversions::toPCL(header, normals.header);

  pcl::Normal normal;
  normal.normal_x = 0.0F;
  normal.normal_y = 1.0F;
  normal.normal_z = 0.0F;
  normal.curvature = 0.25F;
  normals.push_back(normal);
  normals.width = 1;
  normals.height = 1;

  sensor_msgs::msg::PointCloud2 ros_message;
  rclcpp::TypeAdapter<NormalAdapter>::convert_to_ros_message(normals, ros_message);

  NormalCloud result;
  rclcpp::TypeAdapter<NormalAdapter>::convert_to_custom(ros_message, result);

  std_msgs::msg::Header result_header;
  pcl_conversions::fromPCL(result.header, result_header);
  EXPECT_EQ(result_header.frame_id, header.frame_id);
  EXPECT_EQ(result_header.stamp.sec, header.stamp.sec);
  EXPECT_EQ(result_header.stamp.nanosec, header.stamp.nanosec);
  ASSERT_EQ(result.size(), 1U);
  EXPECT_FLOAT_EQ(result.front().normal_x, normal.normal_x);
  EXPECT_FLOAT_EQ(result.front().normal_y, normal.normal_y);
  EXPECT_FLOAT_EQ(result.front().normal_z, normal.normal_z);
  EXPECT_FLOAT_EQ(result.front().curvature, normal.curvature);
}

TEST(PclTypeAdapter, ConvertsPointIndices)
{
  pcl::PointIndices indices;
  const auto header = makeHeader();
  pcl_conversions::toPCL(header, indices.header);
  indices.indices = {1, 4, 9};

  pcl_msgs::msg::PointIndices ros_message;
  rclcpp::TypeAdapter<IndicesAdapter>::convert_to_ros_message(indices, ros_message);

  pcl::PointIndices result;
  rclcpp::TypeAdapter<IndicesAdapter>::convert_to_custom(ros_message, result);

  std_msgs::msg::Header result_header;
  pcl_conversions::fromPCL(result.header, result_header);
  EXPECT_EQ(result_header.frame_id, header.frame_id);
  EXPECT_EQ(result_header.stamp.sec, header.stamp.sec);
  EXPECT_EQ(result_header.stamp.nanosec, header.stamp.nanosec);
  EXPECT_EQ(result.indices, indices.indices);
}

}  // namespace
