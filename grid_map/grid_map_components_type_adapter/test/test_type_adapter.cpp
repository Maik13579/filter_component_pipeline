// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <vector>

#include <grid_map_core/grid_map_core.hpp>
#include <grid_map_msgs/msg/grid_map.hpp>
#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/node.hpp>
#include <rclcpp/type_adapter.hpp>

#include "grid_map_components_type_adapter/ros/grid_map_type_adapter.hpp"

namespace
{

using Adapter = grid_map_components_type_adapter::ros::GridMapAdapter;

grid_map::GridMap makeGridMap()
{
  grid_map::GridMap map({"elevation", "variance"});
  map.setFrameId("map");
  map.setTimestamp(1234567890ULL);
  map.setGeometry(grid_map::Length(2.0, 2.0), 1.0, grid_map::Position(0.5, -0.5));
  map.setBasicLayers({"elevation"});

  map.at("elevation", grid_map::Index(0, 0)) = 1.25F;
  map.at("elevation", grid_map::Index(0, 1)) = 2.5F;
  map.at("elevation", grid_map::Index(1, 0)) = -3.75F;
  map.at("variance", grid_map::Index(0, 0)) = 0.1F;
  map.at("variance", grid_map::Index(1, 1)) = 0.4F;

  return map;
}

void expectSameMetadata(const grid_map::GridMap & result, const grid_map::GridMap & original)
{
  EXPECT_EQ(result.getFrameId(), original.getFrameId());
  EXPECT_EQ(result.getTimestamp(), original.getTimestamp());
  EXPECT_DOUBLE_EQ(result.getResolution(), original.getResolution());
  EXPECT_TRUE(result.getLength().isApprox(original.getLength()));
  EXPECT_TRUE(result.getPosition().isApprox(original.getPosition()));
  EXPECT_TRUE((result.getSize() == original.getSize()).all());
  EXPECT_EQ(result.getLayers(), original.getLayers());
  EXPECT_EQ(result.getBasicLayers(), original.getBasicLayers());
}

TEST(GridMapTypeAdapter, PreservesMetadataLayersAndValuesRoundTrip)
{
  const auto original = makeGridMap();

  grid_map_msgs::msg::GridMap ros_message;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(original, ros_message);

  grid_map::GridMap result;
  rclcpp::TypeAdapter<Adapter>::convert_to_custom(ros_message, result);

  expectSameMetadata(result, original);
  EXPECT_FLOAT_EQ(result.at("elevation", grid_map::Index(0, 0)), 1.25F);
  EXPECT_FLOAT_EQ(result.at("elevation", grid_map::Index(0, 1)), 2.5F);
  EXPECT_FLOAT_EQ(result.at("elevation", grid_map::Index(1, 0)), -3.75F);
  EXPECT_FLOAT_EQ(result.at("variance", grid_map::Index(0, 0)), 0.1F);
  EXPECT_FLOAT_EQ(result.at("variance", grid_map::Index(1, 1)), 0.4F);
}

TEST(GridMapTypeAdapter, KeepsRosMetadataUnchangedAfterCustomRoundTrip)
{
  const auto original = makeGridMap();

  grid_map_msgs::msg::GridMap original_ros;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(original, original_ros);

  grid_map::GridMap custom;
  rclcpp::TypeAdapter<Adapter>::convert_to_custom(original_ros, custom);

  grid_map_msgs::msg::GridMap round_trip;
  rclcpp::TypeAdapter<Adapter>::convert_to_ros_message(custom, round_trip);

  EXPECT_EQ(round_trip.header.frame_id, original_ros.header.frame_id);
  EXPECT_EQ(round_trip.header.stamp.sec, original_ros.header.stamp.sec);
  EXPECT_EQ(round_trip.header.stamp.nanosec, original_ros.header.stamp.nanosec);
  EXPECT_EQ(round_trip.info.resolution, original_ros.info.resolution);
  EXPECT_EQ(round_trip.layers, original_ros.layers);
  EXPECT_EQ(round_trip.basic_layers, original_ros.basic_layers);
}

TEST(GridMapTypeAdapter, IntraProcessUniquePtrPreservesGridMapAddress)
{
  const auto log_dir = std::filesystem::path{"/tmp/grid_map_components_type_adapter_test_logs"};
  std::filesystem::create_directories(log_dir);
  setenv("ROS_LOG_DIR", log_dir.c_str(), 1);
  setenv("ROS_AUTOMATIC_DISCOVERY_RANGE", "LOCALHOST", 1);

  int argc = 0;
  char ** argv = nullptr;
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
    "grid_map_type_adapter_zero_copy_test",
    rclcpp::NodeOptions().use_intra_process_comms(true));

  grid_map::GridMap * published_message = nullptr;
  grid_map::GridMap * received_message = nullptr;

  auto subscription = node->create_subscription<Adapter>(
    "map",
    rclcpp::QoS(1),
    [&](std::unique_ptr<grid_map::GridMap> message) {
      received_message = message.get();
    });

  auto publisher = node->create_publisher<Adapter>("map", rclcpp::QoS(1));

  auto message = std::make_unique<grid_map::GridMap>(makeGridMap());
  published_message = message.get();

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

  subscription.reset();
  publisher.reset();
  node.reset();
  rclcpp::shutdown();
}

}  // namespace
