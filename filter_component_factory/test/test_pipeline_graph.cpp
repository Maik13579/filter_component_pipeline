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
  const auto path = std::string{"/tmp/filter_components_pipeline_test.yaml"};
  std::ofstream stream{path};
  stream << yaml;
  return path;
}

TEST(PipelineGraph, LoadsGraphAndPreservesTypes)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
    position: {x: 1.0, y: 2.0}
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
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
      mode: receipt_time
      max_interval: 0.1
  - type: topic
    topic: /filtered
    input_type: PointXYZI
    output_type: PointXYZI
  - type: topic
    topic: /filter_pipeline/voxel_to_output
    input_type: PointXYZI
    output_type: PointXYZI
    qos:
      depth: 7
      reliability: reliable
    position: {x: 120.0, y: 80.0}
edges:
  - from: {node: /points, port: out, direction: output}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: input}
  - from: {node: VoxelGridXYZI_1, port: cloud, direction: output}
    to: {node: /filter_pipeline/voxel_to_output, port: in, direction: input}
  - from: {node: /filter_pipeline/voxel_to_output, port: out, direction: output}
    to: {node: /filtered, port: in, direction: input}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.nodes.size(), 4U);
  EXPECT_EQ(graph.nodes[1].component_class, "test_filter_components::VoxelGridXYZIComponent");
  EXPECT_EQ(graph.nodes[1].input_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].output_type, "PointXYZI");
  EXPECT_EQ(graph.nodes[1].parameters.at("filter.leaf_size_x"), "0.1");
  EXPECT_EQ(graph.nodes[1].inputs.at("cloud").at("depth"), "8");
  EXPECT_EQ(graph.nodes[1].inputs.at("cloud").at("reliability"), "reliable");
  EXPECT_EQ(graph.nodes[1].outputs.at("cloud").at("durability"), "transient_local");
  EXPECT_EQ(graph.nodes[1].sync.at("mode"), "receipt_time");
  EXPECT_EQ(graph.nodes[1].sync.at("max_interval"), "0.1");
  EXPECT_EQ(graph.nodes[3].topic, "/filter_pipeline/voxel_to_output");
  EXPECT_TRUE(graph.nodes[3].qos.empty());
  EXPECT_DOUBLE_EQ(graph.nodes[3].x, 120.0);
  EXPECT_DOUBLE_EQ(graph.nodes[3].y, 80.0);
  ASSERT_EQ(graph.edges.size(), 3U);
  EXPECT_EQ(graph.edges[0].from.direction, "output");
  EXPECT_EQ(graph.edges[0].to.direction, "input");
  EXPECT_EQ(graph.edges[0].to.port, "cloud");
  EXPECT_EQ(graph.edges[1].from.port, "cloud");
}

TEST(PipelineGraph, RejectsRemovedSyncPolicy)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: filter
    name: PointCloudMergerXYZI_1
    package: filter_component_factory
    filter: PointCloudMergerXYZI
    component_class: test_filter_components::PointCloudMergerXYZIComponent
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
    sync:
      policy: ExactTime
edges: []
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsMissingEndpointDirection)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /points, port: out}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsWrongEndpointDirection)
{
  const auto wrong_from_path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /points, port: out, direction: input}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(wrong_from_path),
    std::runtime_error);

  const auto wrong_to_path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /points, port: out, direction: output}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: output}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(wrong_to_path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsVersionOne)
{
  const auto path = writeTempPipeline(R"(
version: 1
nodes: []
edges: []
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsTypeIncompatibleEdges)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /indices
    input_type: PointIndices
    output_type: PointIndices
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /indices, port: out, direction: output}
    to: {node: VoxelGridXYZI_1, port: cloud, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsOriginalCloudOutputPort)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    output_type: PointXYZI
    output_ports: cloud:PointXYZI,orig_cloud:PointXYZI
  - type: topic
    topic: /original
    input_type: PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: VoxelGridXYZI_1, port: orig_cloud, direction: output}
    to: {node: /original, port: in, direction: input}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 1U);
  EXPECT_EQ(graph.edges[0].from.port, "orig_cloud");
}

TEST(PipelineGraph, RejectsMissingFilterComponentClass)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    input_type: PointXYZI
    output_type: PointXYZI
edges: []
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsPythonFilterWithoutComponentClass)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointCloud2
    output_type: PointCloud2
  - type: filter
    name: PythonFilter_1
    python_module: test_filters.python_filter
    python_class: PythonFilter
    input_ports: cloud:PointCloud2
    output_ports: cloud:PointCloud2
  - type: topic
    topic: /filtered
    input_type: PointCloud2
    output_type: PointCloud2
edges:
  - from: {node: /points, port: out, direction: output}
    to: {node: PythonFilter_1, port: cloud, direction: input}
  - from: {node: PythonFilter_1, port: cloud, direction: output}
    to: {node: /filtered, port: in, direction: input}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.nodes.size(), 3U);
  EXPECT_EQ(graph.nodes[1].implementation, "python");
  EXPECT_EQ(graph.nodes[1].python_module, "test_filters.python_filter");
  EXPECT_EQ(graph.nodes[1].python_class, "PythonFilter");
}

TEST(PipelineGraph, RejectsFilterSideLegacyPorts)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: topic
    topic: /points
    input_type: PointXYZI
    output_type: PointXYZI
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
    input_ports: cloud:PointXYZI
    output_ports: cloud:PointXYZI
edges:
  - from: {node: /points, port: out, direction: output}
    to: {node: VoxelGridXYZI_1, port: in, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsRepeatedInputPorts)
{
  const auto path = writeTempPipeline(R"(
version: 2
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
    package: filter_component_factory
    filter: PointCloudMergerXYZI
    component_class: test_filter_components::PointCloudMergerXYZIComponent
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /a, port: out, direction: output}
    to: {node: PointCloudMergerXYZI_1, port: input_1, direction: input}
  - from: {node: /b, port: out, direction: output}
    to: {node: PointCloudMergerXYZI_1, port: input_2, direction: input}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 2U);
  EXPECT_EQ(graph.edges[0].to.port, "input_1");
  EXPECT_EQ(graph.edges[1].to.port, "input_2");
}

TEST(PipelineGraph, RejectsDuplicateFilterInputPort)
{
  const auto path = writeTempPipeline(R"(
version: 2
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
    package: filter_component_factory
    filter: PointCloudMergerXYZI
    component_class: test_filter_components::PointCloudMergerXYZIComponent
    input_type: PointXYZI,PointXYZI
    output_type: PointXYZI
edges:
  - from: {node: /a, port: out, direction: output}
    to: {node: PointCloudMergerXYZI_1, port: input_1, direction: input}
  - from: {node: /b, port: out, direction: output}
    to: {node: PointCloudMergerXYZI_1, port: input_1, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, RejectsDuplicateFilterOutputPort)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
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
  - from: {node: VoxelGridXYZI_1, port: cloud, direction: output}
    to: {node: /a, port: in, direction: input}
  - from: {node: VoxelGridXYZI_1, port: cloud, direction: output}
    to: {node: /b, port: in, direction: input}
)");

  EXPECT_THROW(
    (void)filter_component_factory::pipeline::loadPipelineGraph(path),
    std::runtime_error);
}

TEST(PipelineGraph, AcceptsExplicitRepeatedOutputPorts)
{
  const auto path = writeTempPipeline(R"(
version: 2
nodes:
  - type: filter
    name: VoxelGridXYZI_1
    package: filter_component_factory
    filter: VoxelGridXYZI
    component_class: test_filter_components::VoxelGridXYZIComponent
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
  - from: {node: VoxelGridXYZI_1, port: cloud, direction: output}
    to: {node: /filtered, port: in, direction: input}
  - from: {node: VoxelGridXYZI_1, port: orig_cloud, direction: output}
    to: {node: /original, port: in, direction: input}
)");

  const auto graph = filter_component_factory::pipeline::loadPipelineGraph(path);

  ASSERT_EQ(graph.edges.size(), 2U);
  EXPECT_EQ(graph.nodes[0].output_ports, "cloud:PointXYZI,orig_cloud:PointXYZI");
}

TEST(PipelineGraph, RejectsDuplicateTopicNodes)
{
  const auto path = writeTempPipeline(R"(
version: 2
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
