// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include "pcl_filter_components/pipeline/pipeline_graph.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace pcl_filter_components::pipeline
{

namespace
{

std::string requireString(const YAML::Node & node, const std::string & key)
{
  if (!node[key] || !node[key].IsScalar()) {
    throw std::runtime_error("Missing string field '" + key + "'");
  }
  return node[key].as<std::string>();
}

std::string optionalString(const YAML::Node & node, const std::string & key)
{
  if (!node[key]) {
    return {};
  }
  return node[key].as<std::string>();
}

std::map<std::string, std::string> stringMap(const YAML::Node & node)
{
  std::map<std::string, std::string> values;
  if (!node) {
    return values;
  }
  if (!node.IsMap()) {
    throw std::runtime_error("Expected a mapping");
  }
  for (const auto & entry : node) {
    if (entry.second.IsScalar()) {
      values.emplace(entry.first.as<std::string>(), entry.second.as<std::string>());
    } else {
      values.emplace(entry.first.as<std::string>(), YAML::Dump(entry.second));
    }
  }
  return values;
}

PipelinePort parsePort(const YAML::Node & node)
{
  if (!node || !node.IsMap()) {
    throw std::runtime_error("Edge endpoint must be a mapping");
  }
  PipelinePort port;
  port.node = requireString(node, "node");
  port.port = optionalString(node, "port");
  return port;
}

}  // namespace

std::string defaultComponentClass(const std::string & package_name, const std::string & filter)
{
  if (package_name.empty() || filter.empty()) {
    return {};
  }
  return package_name + "::" + filter + "Component";
}

PipelineGraph loadPipelineGraph(const std::string & path)
{
  const auto root = YAML::LoadFile(path);
  if (!root || !root.IsMap()) {
    throw std::runtime_error("Pipeline YAML must contain a top-level mapping");
  }

  PipelineGraph graph;
  graph.version = root["version"] ? root["version"].as<int>() : 1;
  if (!root["nodes"] || !root["nodes"].IsSequence()) {
    throw std::runtime_error("Pipeline YAML must contain a nodes sequence");
  }
  if (!root["edges"] || !root["edges"].IsSequence()) {
    throw std::runtime_error("Pipeline YAML must contain an edges sequence");
  }

  for (const auto & item : root["nodes"]) {
    PipelineNode node;
    node.id = requireString(item, "id");
    node.type = requireString(item, "type");
    node.package_name = optionalString(item, "package");
    node.filter = optionalString(item, "filter");
    node.component_class = optionalString(item, "component_class");
    node.input_type = optionalString(item, "input_type");
    node.output_type = optionalString(item, "output_type");
    node.optional_output_type = optionalString(item, "optional_output_type");
    node.topic = optionalString(item, "topic");
    node.parameters = stringMap(item["parameters"]);
    node.qos = stringMap(item["qos"]);
    node.sync = stringMap(item["sync"]);
    if (item["position"] && item["position"].IsMap()) {
      node.x = item["position"]["x"] ? item["position"]["x"].as<double>() : 0.0;
      node.y = item["position"]["y"] ? item["position"]["y"].as<double>() : 0.0;
    }
    if (node.component_class.empty()) {
      node.component_class = defaultComponentClass(node.package_name, node.filter);
    }
    graph.nodes.push_back(std::move(node));
  }

  for (const auto & item : root["edges"]) {
    PipelineEdge edge;
    edge.from = parsePort(item["from"]);
    edge.to = parsePort(item["to"]);
    graph.edges.push_back(std::move(edge));
  }

  validatePipelineGraph(graph);
  return graph;
}

void validatePipelineGraph(const PipelineGraph & graph)
{
  if (graph.version != 1) {
    throw std::runtime_error("Unsupported pipeline graph version " + std::to_string(graph.version));
  }

  std::set<std::string> ids;
  for (const auto & node : graph.nodes) {
    if (node.id.empty()) {
      throw std::runtime_error("Pipeline node id must not be empty");
    }
    if (!ids.insert(node.id).second) {
      throw std::runtime_error("Duplicate pipeline node id '" + node.id + "'");
    }
    if (node.type != "input" && node.type != "filter" && node.type != "output") {
      throw std::runtime_error("Unsupported node type '" + node.type + "' on node '" + node.id + "'");
    }
    if (node.type == "filter" && node.component_class.empty()) {
      throw std::runtime_error("Filter node '" + node.id + "' has no component class");
    }
    if ((node.type == "input" || node.type == "output") && node.topic.empty()) {
      throw std::runtime_error("Pseudo node '" + node.id + "' must declare a topic");
    }
  }

  for (const auto & edge : graph.edges) {
    if (ids.count(edge.from.node) == 0U) {
      throw std::runtime_error("Edge references unknown source node '" + edge.from.node + "'");
    }
    if (ids.count(edge.to.node) == 0U) {
      throw std::runtime_error("Edge references unknown target node '" + edge.to.node + "'");
    }
    if (edge.from.node == edge.to.node) {
      throw std::runtime_error("Self edges are not valid for node '" + edge.from.node + "'");
    }
    const auto source = std::find_if(
      graph.nodes.begin(),
      graph.nodes.end(),
      [&edge](const auto & node) {return node.id == edge.from.node;});
    const auto target = std::find_if(
      graph.nodes.begin(),
      graph.nodes.end(),
      [&edge](const auto & node) {return node.id == edge.to.node;});
    const auto source_type = source->type == "input" ? source->output_type : source->output_type;
    const auto target_type = target->type == "output" ? target->input_type : target->input_type;
    if (!source_type.empty() && !target_type.empty() && source_type != target_type) {
      throw std::runtime_error(
        "Type-incompatible edge from '" + edge.from.node + "' (" + source_type + ") to '" +
        edge.to.node + "' (" + target_type + ")");
    }
  }
}

}  // namespace pcl_filter_components::pipeline
