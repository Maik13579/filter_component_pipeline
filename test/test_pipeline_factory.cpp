// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

#include <rclcpp/context.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/node_options.hpp>

#include "pcl_filter_components/pipeline/pipeline_factory_node.hpp"

namespace
{

std::string writeFactoryPipeline()
{
  const auto path = std::string{"/tmp/pcl_filter_components_factory_test.yaml"};
  std::ofstream stream{path};
  stream << R"(
version: 1
nodes:
  - id: input_1
    type: input
    topic: /points/input
    output_type: PointXYZI
  - id: voxel_1
    type: filter
    package: pcl_filter_components
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
    optional_output_type: PointIndices
    parameters:
      filter.leaf_size_x: 0.1
      filter.leaf_size_y: 0.1
      filter.leaf_size_z: 0.1
    qos:
      depth: 5
  - id: output_1
    type: output
    topic: /points/output
    input_type: PointXYZI
edges:
  - from: {node: input_1, port: out}
    to: {node: voxel_1, port: in}
  - from: {node: voxel_1, port: out}
    to: {node: output_1, port: in}
)";
  return path;
}

TEST(PipelineFactoryNode, LoadsAndControlsSingleFilterPipeline)
{
  auto context = std::make_shared<rclcpp::Context>();
  char const * argv[] = {"test_pipeline_factory"};
  context->init(1, argv);

  rclcpp::ExecutorOptions executor_options;
  executor_options.context = context;
  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>(
    executor_options,
    2,
    false);
  const auto pipeline_file = writeFactoryPipeline();
  auto factory = std::make_shared<pcl_filter_components::pipeline::PipelineFactoryNode>(
    executor,
    rclcpp::NodeOptions{}
    .context(context)
    .use_intra_process_comms(true)
    .parameter_overrides({rclcpp::Parameter{"pipeline_file", pipeline_file}}));

  executor->add_node(factory->get_node_base_interface());

  pcl_filter_components::pipeline::PipelineFactoryNode::CallbackReturn callback_return;
  factory->configure(callback_return);
  EXPECT_EQ(callback_return, pcl_filter_components::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->activate(callback_return);
  EXPECT_EQ(callback_return, pcl_filter_components::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->deactivate(callback_return);
  EXPECT_EQ(callback_return, pcl_filter_components::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->cleanup(callback_return);
  EXPECT_EQ(callback_return, pcl_filter_components::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  executor->remove_node(factory->get_node_base_interface());
  factory.reset();
  executor.reset();
  context->shutdown("test complete");
}

}  // namespace
