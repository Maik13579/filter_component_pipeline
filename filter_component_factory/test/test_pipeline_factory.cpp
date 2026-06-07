// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

#include <rclcpp/context.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/node_options.hpp>

#include "filter_component_factory/pipeline/pipeline_factory_node.hpp"
#include "filter_component_factory/pipeline/pipeline_graph.hpp"

namespace
{

std::string writeFactoryPipeline()
{
  const auto path = std::string{"/tmp/pcl_filter_components_factory_test.yaml"};
  std::ofstream stream{path};
  stream << R"(
version: 1
nodes:
  - type: topic
    topic: /points/input
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
    parameters:
      filter.leaf_size_x: 0.1
      filter.leaf_size_y: 0.1
      filter.leaf_size_z: 0.1
  - type: topic
    topic: /pcl_pipeline/voxel_filtered
    input_type: PointXYZI
    output_type: PointXYZI
    qos:
      depth: 5
      reliability: best_effort
  - type: topic
    topic: /points/output
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /points/input, port: out}
    to: {node: VoxelGridXYZI_1, port: in}
  - from: {node: VoxelGridXYZI_1, port: out}
    to: {node: /pcl_pipeline/voxel_filtered, port: in}
  - from: {node: /pcl_pipeline/voxel_filtered, port: out}
    to: {node: /points/output, port: in}
)";
  return path;
}

std::string writeMergerPipeline()
{
  const auto path = std::string{"/tmp/pcl_filter_components_merger_factory_test.yaml"};
  std::ofstream stream{path};
  stream << R"(
version: 1
nodes:
  - type: topic
    topic: /points/input_a
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /points/input_b
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: PointCloudMergerXYZI_1
    package: pcl_filter_components_xyzi
    filter: PointCloudMergerXYZI
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /points/merged
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /points/input_a, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_1}
  - from: {node: /points/input_b, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_2}
  - from: {node: PointCloudMergerXYZI_1, port: out}
    to: {node: /points/merged, port: in}
)";
  return path;
}

void expectFactoryLoadsPipeline(const std::string & pipeline_file)
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
  auto factory = std::make_shared<filter_component_factory::pipeline::PipelineFactoryNode>(
    executor,
    rclcpp::NodeOptions{}
    .context(context)
    .use_intra_process_comms(true)
    .parameter_overrides({rclcpp::Parameter{"pipeline_file", pipeline_file}}));

  executor->add_node(factory->get_node_base_interface());

  filter_component_factory::pipeline::PipelineFactoryNode::CallbackReturn callback_return;
  factory->configure(callback_return);
  EXPECT_EQ(callback_return, filter_component_factory::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->activate(callback_return);
  EXPECT_EQ(callback_return, filter_component_factory::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->deactivate(callback_return);
  EXPECT_EQ(callback_return, filter_component_factory::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  factory->cleanup(callback_return);
  EXPECT_EQ(callback_return, filter_component_factory::pipeline::PipelineFactoryNode::CallbackReturn::SUCCESS);

  executor->remove_node(factory->get_node_base_interface());
  factory.reset();
  executor.reset();
  context->shutdown("test complete");
}

TEST(PipelineFactoryNode, LoadsAndControlsSingleFilterPipeline)
{
  expectFactoryLoadsPipeline(writeFactoryPipeline());
}

TEST(PipelineFactoryNode, LoadsAndControlsMergerPipeline)
{
  expectFactoryLoadsPipeline(writeMergerPipeline());
}

TEST(PipelineFactoryNode, LoadsInstalledExamplePipeline)
{
  const auto pipeline_file = std::string{PROJECT_SOURCE_DIR} + "/config/example_pipeline.yaml";
  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(pipeline_file);

  ASSERT_EQ(graph.nodes.size(), 5U);
  ASSERT_EQ(graph.edges.size(), 4U);
  EXPECT_EQ(graph.nodes[1].id, "VoxelGridXYZI_1");
  EXPECT_EQ(graph.nodes[2].type, "topic");
  EXPECT_EQ(graph.nodes[2].id, "/pcl_pipeline/voxel_filtered");
  EXPECT_TRUE(graph.nodes[2].qos.empty());

  expectFactoryLoadsPipeline(pipeline_file);
}

}  // namespace
