// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "filter_component_factory/pipeline/pipeline_graph.hpp"

namespace
{

std::string writeTempPipeline(const std::string & yaml)
{
  const auto path = std::string{"/tmp/pcl_filter_components_pipeline_test.yaml"};
  std::ofstream stream{path};
  stream << yaml;
  return path;
}

TEST(PipelineGraph, LoadsGraphAndPreservesTypes)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
    position: {x: 1.0, y: 2.0}
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
    parameters:
      filter.leaf_size_x: 0.1
    inputs:
      cloud:
        qos: {reliability: reliable, history: keep_last, depth: 8, durability: volatile}
    outputs:
      cloud:
        qos: {durability: transient_local}
    sync:
      policy: ExactTime
  - type: topic
    topic: /filtered
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /pcl_pipeline/voxel_to_output
    input_type: PointXYZI
    output_type: PointXYZI
    qos:
      depth: 7
      reliability: reliable
    position: {x: 120.0, y: 80.0}
edges:
  - from: {node: /points, port: out}
    to: {node: VoxelGridXYZI_1, port: in}
  - from: {node: VoxelGridXYZI_1, port: out}
    to: {node: /pcl_pipeline/voxel_to_output, port: in}
  - from: {node: /pcl_pipeline/voxel_to_output, port: out}
    to: {node: /filtered, port: in}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.nodes.size(), 4U);
  EXPECT_EQ(graph.nodes[1].component_class, "pcl_filter_components_xyzi::VoxelGridXYZIComponent");
  EXPECT_EQ(graph.nodes[1].input_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].output_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].parameters.at("filter.leaf_size_x"), "0.1");
  EXPECT_EQ(graph.nodes[1].inputs.at("cloud").at("depth"), "8");
  EXPECT_EQ(graph.nodes[1].inputs.at("cloud").at("reliability"), "reliable");
  EXPECT_EQ(graph.nodes[1].outputs.at("cloud").at("durability"), "transient_local");
  EXPECT_EQ(graph.nodes[1].sync.at("policy"), "ExactTime");
  EXPECT_EQ(graph.nodes[3].topic, "/pcl_pipeline/voxel_to_output");
  EXPECT_TRUE(graph.nodes[3].qos.empty());
  EXPECT_DOUBLE_EQ(graph.nodes[3].x, 120.0);
  EXPECT_DOUBLE_EQ(graph.nodes[3].y, 80.0);
  ASSERT_EQ(graph.edges.size(), 3U);
}

TEST(PipelineGraph, RejectsTypeIncompatibleEdges)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: topic
    topic: /indices
    input_type: PointIndices
    output_type: PointIndices
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /indices}
    to: {node: VoxelGridXYZI_1}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsOriginalCloudOutputPort)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    output_type: PointXYZI
  - type: topic
    topic: /original
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: VoxelGridXYZI_1, port: orig_cloud}
    to: {node: /original, port: in}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 1U);
  EXPECT_EQ(graph.edges[0].from.port, "orig_cloud");
}

TEST(PipelineGraph, AcceptsRepeatedInputPorts)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: topic
    topic: /a
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /b
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: PointCloudMergerXYZI_1
    package: pcl_filter_components_xyzi
    filter: PointCloudMergerXYZI
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /a, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_1}
  - from: {node: /b, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_2}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 2U);
  EXPECT_EQ(graph.edges[0].to.port, "input_1");
  EXPECT_EQ(graph.edges[1].to.port, "input_2");
}

TEST(PipelineGraph, RejectsDuplicateFilterInputPort)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: topic
    topic: /a
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /b
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: PointCloudMergerXYZI_1
    package: pcl_filter_components_xyzi
    filter: PointCloudMergerXYZI
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /a, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_1}
  - from: {node: /b, port: out}
    to: {node: PointCloudMergerXYZI_1, port: input_1}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsDuplicateFilterOutputPort)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /a
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /b
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: VoxelGridXYZI_1, port: out}
    to: {node: /a, port: in}
  - from: {node: VoxelGridXYZI_1, port: out}
    to: {node: /b, port: in}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsExplicitRepeatedOutputPorts)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: pcl_filter_components_xyzi
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI,PointXYZI
    output_ports: cloud:PointXYZI,orig_cloud:PointXYZI
  - type: topic
    topic: /filtered
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /original
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: VoxelGridXYZI_1, port: cloud}
    to: {node: /filtered, port: in}
  - from: {node: VoxelGridXYZI_1, port: orig_cloud}
    to: {node: /original, port: in}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 2U);
  EXPECT_EQ(graph.nodes[0].output_ports, "cloud:PointXYZI,orig_cloud:PointXYZI");
}

TEST(PipelineGraph, RejectsDuplicateTopicNodes)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - type: topic
    topic: /duplicate
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /duplicate
    input_type: PointXYZI
    output_type: PointXYZI
edges: []
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

}  // namespace
