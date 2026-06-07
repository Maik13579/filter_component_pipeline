// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_FACTORY__PIPELINE__PIPELINE_GRAPH_HPP_
#define FILTER_COMPONENT_FACTORY__PIPELINE__PIPELINE_GRAPH_HPP_

#include <map>
#include <string>
#include <vector>

namespace filter_component_factory::pipeline
{

struct PipelinePort
{
  std::string node;
  std::string port;
};

struct PipelineEdge
{
  PipelinePort from;
  PipelinePort to;
  std::string topic;
  std::map<std::string, std::string> qos;
  double x{0.0};
  double y{0.0};
};

struct PipelineNode
{
  std::string id;
  std::string type;
  std::string name;
  std::string package_name;
  std::string filter;
  std::string component_class;
  std::string input_type;
  std::string output_type;
  std::string input_ports;
  std::string output_ports;
  std::string topic;
  std::map<std::string, std::string> parameters;
  std::map<std::string, std::string> qos;
  std::map<std::string, std::map<std::string, std::string>> inputs;
  std::map<std::string, std::map<std::string, std::string>> outputs;
  std::map<std::string, std::string> sync;
  double x{0.0};
  double y{0.0};
};

struct PipelineGraph
{
  int version{1};
  std::vector<PipelineNode> nodes;
  std::vector<PipelineEdge> edges;
};

PipelineGraph loadPipelineGraph(const std::string & path);
void validatePipelineGraph(const PipelineGraph & graph);
std::string defaultComponentClass(const std::string & package_name, const std::string & filter);

}  // namespace filter_component_factory::pipeline

#endif  // FILTER_COMPONENT_FACTORY__PIPELINE__PIPELINE_GRAPH_HPP_
