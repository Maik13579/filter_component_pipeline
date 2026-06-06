// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "pcl_filter_components/pipeline/pipeline_graph.hpp"

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
  - id: input_1
    type: input
    topic: /points
    output_type: PointXYZI
    position: {x: 1.0, y: 2.0}
  - id: voxel_1
    type: filter
    package: pcl_filter_components
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
    optional_output_type: PointIndices
    parameters:
      filter.leaf_size_x: 0.1
    sync:
      policy: ExactTime
  - id: output_1
    type: output
    topic: /filtered
    input_type: PointXYZI
  - id: topic_1
    type: topic
    topic: /pcl_pipeline/voxel_to_output
    input_type: PointXYZI
    output_type: PointXYZI
    qos:
      depth: 7
      reliability: reliable
    position: {x: 120.0, y: 80.0}
edges:
  - from: {node: input_1, port: out}
    to: {node: voxel_1, port: in}
  - from: {node: voxel_1, port: out}
    to: {node: topic_1, port: in}
  - from: {node: topic_1, port: out}
    to: {node: output_1, port: in}
)");

  const auto graph = pcl_filter_components::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.nodes.size(), 4U);
  EXPECT_EQ(graph.nodes[1].component_class, "pcl_filter_components::VoxelGridXYZIComponent");
  EXPECT_EQ(graph.nodes[1].input_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].output_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].optional_output_type, "PointIndices");
  EXPECT_EQ(graph.nodes[1].parameters.at("filter.leaf_size_x"), "0.1");
  EXPECT_EQ(graph.nodes[1].sync.at("policy"), "ExactTime");
  EXPECT_EQ(graph.nodes[3].topic, "/pcl_pipeline/voxel_to_output");
  EXPECT_EQ(graph.nodes[3].qos.at("depth"), "7");
  EXPECT_EQ(graph.nodes[3].qos.at("reliability"), "reliable");
  EXPECT_DOUBLE_EQ(graph.nodes[3].x, 120.0);
  EXPECT_DOUBLE_EQ(graph.nodes[3].y, 80.0);
  ASSERT_EQ(graph.edges.size(), 3U);
}

TEST(PipelineGraph, RejectsTypeIncompatibleEdges)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - id: input_1
    type: input
    topic: /indices
    output_type: PointIndices
  - id: voxel_1
    type: filter
    package: pcl_filter_components
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: input_1}
    to: {node: voxel_1}
)");

  EXPECT_THROW(
    (void)pcl_filter_components::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsDuplicateTopicNodes)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes:
  - id: topic_1
    type: topic
    topic: /duplicate
    input_type: PointXYZI
    output_type: PointXYZI
  - id: topic_2
    type: topic
    topic: /duplicate
    input_type: PointXYZI
    output_type: PointXYZI
edges: []
)");

  EXPECT_THROW(
    (void)pcl_filter_components::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

}  // namespace
