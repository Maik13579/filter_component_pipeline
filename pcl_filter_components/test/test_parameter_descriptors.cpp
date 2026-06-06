// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include <pcl/point_types.h>
#include <rclcpp/context.hpp>
#include <rclcpp/node_options.hpp>

#include "pcl_filter_components/ros/crop_box_component.hpp"
#include "pcl_filter_components/ros/passthrough_component.hpp"
#include "pcl_filter_components/ros/point_cloud_merger_component.hpp"
#include "pcl_filter_components/ros/voxel_grid_component.hpp"

namespace
{

template <typename ComponentT>
std::shared_ptr<ComponentT> makeComponent(std::shared_ptr<rclcpp::Context> context)
{
  char const * argv[] = {"test_parameter_descriptors"};
  context->init(1, argv);

  rclcpp::NodeOptions options;
  options.context(context);
  return std::make_shared<ComponentT>(options);
}

template <typename NodeT>
void expectDeclaredWithDescription(
  const NodeT & node,
  const std::vector<std::string> & parameter_names)
{
  for (const auto & name : parameter_names) {
    EXPECT_TRUE(node.has_parameter(name)) << name;
    EXPECT_FALSE(node.describe_parameter(name).description.empty()) << name;
  }
}

template <typename NodeT>
void expectFloatingPointRange(const NodeT & node, const std::string & parameter_name)
{
  const auto descriptor = node.describe_parameter(parameter_name);
  ASSERT_EQ(descriptor.floating_point_range.size(), 1U) << parameter_name;
  EXPECT_LT(
    descriptor.floating_point_range.front().from_value,
    descriptor.floating_point_range.front().to_value)
    << parameter_name;
}

template <typename NodeT>
void expectIntegerRange(const NodeT & node, const std::string & parameter_name)
{
  const auto descriptor = node.describe_parameter(parameter_name);
  ASSERT_EQ(descriptor.integer_range.size(), 1U) << parameter_name;
  EXPECT_LT(
    descriptor.integer_range.front().from_value,
    descriptor.integer_range.front().to_value)
    << parameter_name;
}

template <typename NodeT>
void expectCommonParameterMetadata(const NodeT & node)
{
  expectDeclaredWithDescription(
    node,
    {
      "inputs.cloud.topic",
      "inputs.cloud.qos.reliability",
      "inputs.cloud.qos.history",
      "inputs.cloud.qos.depth",
      "inputs.cloud.qos.durability",
      "outputs.cloud.topic",
      "outputs.cloud.qos.reliability",
      "outputs.cloud.qos.history",
      "outputs.cloud.qos.depth",
      "outputs.cloud.qos.durability",
      "outputs.indices.topic",
      "outputs.indices.qos.reliability",
      "outputs.indices.qos.history",
      "outputs.indices.qos.depth",
      "outputs.indices.qos.durability",
      "filter.output_indices",
    });
  expectIntegerRange(node, "inputs.cloud.qos.depth");
  expectIntegerRange(node, "outputs.cloud.qos.depth");
  expectIntegerRange(node, "outputs.indices.qos.depth");

  EXPECT_EQ(node.get_parameter("inputs.cloud.topic").as_string(), "/points/input");
  EXPECT_EQ(node.get_parameter("inputs.cloud.qos.reliability").as_string(), "best_effort");
  EXPECT_EQ(node.get_parameter("inputs.cloud.qos.history").as_string(), "keep_last");
  EXPECT_EQ(node.get_parameter("inputs.cloud.qos.depth").as_int(), 5);
  EXPECT_EQ(node.get_parameter("inputs.cloud.qos.durability").as_string(), "volatile");
  EXPECT_EQ(node.get_parameter("outputs.cloud.topic").as_string(), "/points/output");
  EXPECT_EQ(node.get_parameter("outputs.indices.topic").as_string(), "/points/indices");
  EXPECT_FALSE(node.get_parameter("filter.output_indices").as_bool());
  EXPECT_FALSE(node.has_parameter("queue_size"));
  EXPECT_FALSE(node.has_parameter("sync.policy"));
  EXPECT_FALSE(node.has_parameter("sync.queue_size"));
  EXPECT_FALSE(node.has_parameter("sync.slop"));
}

TEST(ParameterDescriptors, VoxelGridXYZIParametersExposeEditorMetadata)
{
  using Component = pcl_filter_components::ros::VoxelGridComponent<pcl::PointXYZI>;

  auto context = std::make_shared<rclcpp::Context>();
  auto node = makeComponent<Component>(context);

  expectCommonParameterMetadata(*node);
  expectDeclaredWithDescription(
    *node,
    {
      "filter.leaf_size_x",
      "filter.leaf_size_y",
      "filter.leaf_size_z",
      "filter.invert",
    });
  expectFloatingPointRange(*node, "filter.leaf_size_x");
  expectFloatingPointRange(*node, "filter.leaf_size_y");
  expectFloatingPointRange(*node, "filter.leaf_size_z");

  EXPECT_DOUBLE_EQ(node->get_parameter("filter.leaf_size_x").as_double(), 0.05);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.leaf_size_y").as_double(), 0.05);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.leaf_size_z").as_double(), 0.05);
  EXPECT_FALSE(node->get_parameter("filter.invert").as_bool());

  node.reset();
  context->shutdown("test complete");
}

TEST(ParameterDescriptors, PassThroughXYZIParametersExposeEditorMetadata)
{
  using Component = pcl_filter_components::ros::PassThroughComponent<pcl::PointXYZI>;

  auto context = std::make_shared<rclcpp::Context>();
  auto node = makeComponent<Component>(context);

  expectCommonParameterMetadata(*node);
  expectDeclaredWithDescription(
    *node,
    {
      "filter.field_name",
      "filter.min_value",
      "filter.max_value",
      "filter.invert",
    });
  expectFloatingPointRange(*node, "filter.min_value");
  expectFloatingPointRange(*node, "filter.max_value");

  EXPECT_EQ(node->get_parameter("filter.field_name").as_string(), "z");
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.min_value").as_double(), -1.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.max_value").as_double(), 2.0);
  EXPECT_FALSE(node->get_parameter("filter.invert").as_bool());

  node.reset();
  context->shutdown("test complete");
}

TEST(ParameterDescriptors, CropBoxXYZIParametersExposeEditorMetadata)
{
  using Component = pcl_filter_components::ros::CropBoxComponent<pcl::PointXYZI>;

  auto context = std::make_shared<rclcpp::Context>();
  auto node = makeComponent<Component>(context);

  expectCommonParameterMetadata(*node);
  expectDeclaredWithDescription(
    *node,
    {
      "filter.min_x",
      "filter.min_y",
      "filter.min_z",
      "filter.max_x",
      "filter.max_y",
      "filter.max_z",
      "filter.invert",
    });
  for (const auto & name : {
    "filter.min_x",
    "filter.min_y",
    "filter.min_z",
    "filter.max_x",
    "filter.max_y",
    "filter.max_z",
  })
  {
    expectFloatingPointRange(*node, name);
  }

  EXPECT_DOUBLE_EQ(node->get_parameter("filter.min_x").as_double(), -10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.min_y").as_double(), -10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.min_z").as_double(), -2.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.max_x").as_double(), 10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.max_y").as_double(), 10.0);
  EXPECT_DOUBLE_EQ(node->get_parameter("filter.max_z").as_double(), 3.0);
  EXPECT_FALSE(node->get_parameter("filter.invert").as_bool());

  node.reset();
  context->shutdown("test complete");
}

TEST(ParameterDescriptors, PointCloudMergerXYZIParametersExposeEditorMetadata)
{
  using Component = pcl_filter_components::ros::PointCloudMergerComponent<pcl::PointXYZI>;

  auto context = std::make_shared<rclcpp::Context>();
  auto node = makeComponent<Component>(context);

  expectDeclaredWithDescription(
    *node,
    {
      "inputs.input_1.topic",
      "inputs.input_1.qos.reliability",
      "inputs.input_1.qos.history",
      "inputs.input_1.qos.depth",
      "inputs.input_1.qos.durability",
      "inputs.input_2.topic",
      "inputs.input_2.qos.reliability",
      "inputs.input_2.qos.history",
      "inputs.input_2.qos.depth",
      "inputs.input_2.qos.durability",
      "outputs.cloud.topic",
      "outputs.cloud.qos.reliability",
      "outputs.cloud.qos.history",
      "outputs.cloud.qos.depth",
      "outputs.cloud.qos.durability",
      "sync.policy",
      "sync.queue_size",
      "sync.slop",
    });
  expectIntegerRange(*node, "inputs.input_1.qos.depth");
  expectIntegerRange(*node, "inputs.input_2.qos.depth");
  expectIntegerRange(*node, "outputs.cloud.qos.depth");
  expectIntegerRange(*node, "sync.queue_size");
  expectFloatingPointRange(*node, "sync.slop");

  EXPECT_EQ(node->get_parameter("inputs.input_1.topic").as_string(), "/points/input_a");
  EXPECT_EQ(node->get_parameter("inputs.input_2.topic").as_string(), "/points/input_b");
  EXPECT_EQ(node->get_parameter("outputs.cloud.topic").as_string(), "/points/output");
  EXPECT_EQ(node->get_parameter("inputs.input_1.qos.reliability").as_string(), "best_effort");
  EXPECT_EQ(node->get_parameter("inputs.input_2.qos.depth").as_int(), 5);
  EXPECT_EQ(node->get_parameter("outputs.cloud.qos.durability").as_string(), "volatile");
  EXPECT_FALSE(node->has_parameter("queue_size"));
  EXPECT_FALSE(node->has_parameter("filter.output_indices"));
  EXPECT_EQ(node->get_parameter("sync.policy").as_string(), "ExactTime");
  EXPECT_EQ(node->get_parameter("sync.queue_size").as_int(), 10);
  EXPECT_DOUBLE_EQ(node->get_parameter("sync.slop").as_double(), 0.05);

  node.reset();
  context->shutdown("test complete");
}

}  // namespace
