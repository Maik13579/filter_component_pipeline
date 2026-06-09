// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include "filter_component_factory/pipeline/pipeline_graph.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace filter_component_factory::pipeline
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

std::map<std::string, std::map<std::string, std::string>> portConfigMap(const YAML::Node & node)
{
  std::map<std::string, std::map<std::string, std::string>> values;
  if (!node) {
    return values;
  }
  if (!node.IsMap()) {
    throw std::runtime_error("Expected a port configuration mapping");
  }
  for (const auto & entry : node) {
    const auto port_name = entry.first.as<std::string>();
    const auto qos = entry.second["qos"] ? stringMap(entry.second["qos"]) :
      std::map<std::string, std::string>{};
    values.emplace(port_name, qos);
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
  port.direction = requireString(node, "direction");
  return port;
}

std::vector<std::string> splitTypes(const std::string & value)
{
  std::vector<std::string> types;
  std::stringstream stream{value};
  std::string item;
  while (std::getline(stream, item, ',')) {
    const auto first = item.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
      continue;
    }
    const auto last = item.find_last_not_of(" \t\n\r");
    types.push_back(item.substr(first, last - first + 1U));
  }
  return types;
}

std::vector<std::pair<std::string, std::string>> splitPorts(const std::string & value)
{
  std::vector<std::pair<std::string, std::string>> ports;
  for (const auto & item : splitTypes(value)) {
    const auto separator = item.find(':');
    if (separator == std::string::npos) {
      ports.push_back({"", item});
      continue;
    }
    auto name = item.substr(0, separator);
    auto stream_type = item.substr(separator + 1U);
    const auto name_first = name.find_first_not_of(" \t\n\r");
    const auto name_last = name.find_last_not_of(" \t\n\r");
    name = name_first == std::string::npos ? std::string{} :
      name.substr(name_first, name_last - name_first + 1U);
    const auto type_first = stream_type.find_first_not_of(" \t\n\r");
    const auto type_last = stream_type.find_last_not_of(" \t\n\r");
    stream_type = type_first == std::string::npos ? std::string{} :
      stream_type.substr(type_first, type_last - type_first + 1U);
    if (!stream_type.empty()) {
      ports.push_back({name, stream_type});
    }
  }
  return ports;
}

std::string portNameForType(
  const std::string & stream_type,
  size_t index,
  size_t total,
  bool outgoing)
{
  if (!outgoing && total > 1U) {
    return "input_" + std::to_string(index + 1U);
  }
  if (stream_type == "PointIndices") {
    return "indices";
  }
  if (stream_type.rfind("Point", 0) == 0) {
    return "cloud";
  }
  auto name = stream_type;
  std::replace(name.begin(), name.end(), '/', '_');
  std::replace(name.begin(), name.end(), ':', '_');
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char value) {
      return static_cast<char>(std::tolower(value));
    });
  return name.empty() ? "out" : name;
}

std::string typeForTopicPort(const std::string & value, const std::string & port, bool outgoing)
{
  const auto ports = splitPorts(value);
  if (ports.empty()) {
    return {};
  }
  if (port.empty() || port == "in" || port == "out") {
    return ports.front().second;
  }
  for (size_t index = 0; index < ports.size(); ++index) {
    const auto & port_name = ports[index].first;
    const auto & stream_type = ports[index].second;
    if (
      port == stream_type ||
      port == port_name ||
      port == portNameForType(stream_type, index, ports.size(), outgoing))
    {
      return stream_type;
    }
  }
  return outgoing ? ports.front().second : std::string{};
}

std::string typeForFilterPort(const std::string & value, const std::string & port, bool outgoing)
{
  const auto ports = splitPorts(value);
  if (ports.empty() || port.empty() || port == "in" || port == "out") {
    return {};
  }
  for (size_t index = 0; index < ports.size(); ++index) {
    const auto & port_name = ports[index].first;
    const auto & stream_type = ports[index].second;
    if (
      port == stream_type ||
      port == port_name ||
      port == portNameForType(stream_type, index, ports.size(), outgoing))
    {
      return stream_type;
    }
  }
  return {};
}

bool filterPortIsValid(const PipelineNode & node, const std::string & port, bool outgoing)
{
  const auto spec = outgoing ?
    (node.output_ports.empty() ? node.output_type : node.output_ports) :
    (node.input_ports.empty() ? node.input_type : node.input_ports);
  return !typeForFilterPort(spec, port, outgoing).empty();
}

std::string canonicalPort(const std::string & value, const std::string & port, bool outgoing)
{
  const auto ports = splitPorts(value);
  const auto default_port = outgoing ? std::string{"out"} : std::string{"in"};
  if (ports.empty()) {
    return port.empty() ? default_port : port;
  }
  if (port.empty() || port == "in" || port == "out") {
    if (!ports.front().first.empty()) {
      return ports.front().first;
    }
    return ports.size() > 1U ? portNameForType(ports.front().second, 0U, ports.size(), outgoing) : default_port;
  }
  for (size_t index = 0; index < ports.size(); ++index) {
    const auto & port_name = ports[index].first;
    const auto inferred_port = portNameForType(ports[index].second, index, ports.size(), outgoing);
    if (port == (port_name.empty() ? inferred_port : port_name)) {
      return port;
    }
  }
  return port;
}

std::string nodeTypeForEdge(const PipelineNode & node, bool outgoing, const std::string & port)
{
  if (node.type == "topic") {
    return typeForTopicPort(node.output_type.empty() ? node.input_type : node.output_type, port, outgoing);
  }
  return typeForFilterPort(
    outgoing ?
    (node.output_ports.empty() ? node.output_type : node.output_ports) :
    (node.input_ports.empty() ? node.input_type : node.input_ports),
    port,
    outgoing);
}

void validateSyncConfig(const PipelineNode & node)
{
  for (const auto & [key, value] : node.sync) {
    (void)value;
    if (key == "policy") {
      throw std::runtime_error(
        "Filter node '" + node.id + "' uses removed sync.policy; use sync.mode instead");
    }
    if (key == "slop") {
      throw std::runtime_error(
        "Filter node '" + node.id + "' uses removed sync.slop; use sync.max_interval instead");
    }
    if (key != "mode" && key != "queue_size" && key != "max_interval") {
      throw std::runtime_error("Unsupported sync option '" + key + "' on filter node '" + node.id + "'");
    }
  }
  const auto mode = node.sync.find("mode");
  if (mode != node.sync.end() && mode->second != "receipt_time" && mode->second != "latest") {
    throw std::runtime_error(
      "Unsupported sync.mode '" + mode->second + "' on filter node '" + node.id + "'");
  }
}

}  // namespace

PipelineGraph loadPipelineGraph(const std::string & path)
{
  const auto root = YAML::LoadFile(path);
  if (!root || !root.IsMap()) {
    throw std::runtime_error("Pipeline YAML must contain a top-level mapping");
  }

  PipelineGraph graph;
  graph.version = root["version"] ? root["version"].as<int>() : 0;
  if (!root["nodes"] || !root["nodes"].IsSequence()) {
    throw std::runtime_error("Pipeline YAML must contain a nodes sequence");
  }
  if (!root["edges"] || !root["edges"].IsSequence()) {
    throw std::runtime_error("Pipeline YAML must contain an edges sequence");
  }

  for (const auto & item : root["nodes"]) {
    PipelineNode node;
    node.type = requireString(item, "type");
    node.name = optionalString(item, "name");
    node.package_name = optionalString(item, "package");
    node.filter = optionalString(item, "filter");
    node.component_class = optionalString(item, "component_class");
    node.python_module = optionalString(item, "python_module");
    node.python_class = optionalString(item, "python_class");
    node.implementation = optionalString(item, "implementation");
    if (node.implementation.empty()) {
      node.implementation =
        (!node.python_module.empty() || !node.python_class.empty()) ? "python" : "cpp";
    }
    node.input_type = optionalString(item, "input_type");
    node.output_type = optionalString(item, "output_type");
    node.input_ports = optionalString(item, "input_ports");
    node.output_ports = optionalString(item, "output_ports");
    node.topic = optionalString(item, "topic");
    if (item["id"]) {
      node.id = requireString(item, "id");
    } else if (!node.name.empty()) {
      node.id = node.name;
    } else {
      node.id = node.topic;
    }
    node.parameters = stringMap(item["parameters"]);
    node.qos = {};
    node.inputs = portConfigMap(item["inputs"]);
    node.outputs = portConfigMap(item["outputs"]);
    node.sync = stringMap(item["sync"]);
    if (item["position"] && item["position"].IsMap()) {
      node.x = item["position"]["x"] ? item["position"]["x"].as<double>() : 0.0;
      node.y = item["position"]["y"] ? item["position"]["y"].as<double>() : 0.0;
    }
    graph.nodes.push_back(std::move(node));
  }

  for (const auto & item : root["edges"]) {
    PipelineEdge edge;
    edge.from = parsePort(item["from"]);
    edge.to = parsePort(item["to"]);
    edge.topic = optionalString(item, "topic");
    edge.qos = {};
    if (item["position"] && item["position"].IsMap()) {
      edge.x = item["position"]["x"] ? item["position"]["x"].as<double>() : 0.0;
      edge.y = item["position"]["y"] ? item["position"]["y"].as<double>() : 0.0;
    }
    graph.edges.push_back(std::move(edge));
  }

  validatePipelineGraph(graph);
  return graph;
}

void validatePipelineGraph(const PipelineGraph & graph)
{
  if (graph.version != 2) {
    throw std::runtime_error("Unsupported pipeline graph version " + std::to_string(graph.version));
  }

  std::set<std::string> ids;
  std::set<std::string> topic_names;
  for (const auto & node : graph.nodes) {
    if (node.id.empty()) {
      throw std::runtime_error("Pipeline node id must not be empty");
    }
    if (!ids.insert(node.id).second) {
      throw std::runtime_error("Duplicate pipeline node id '" + node.id + "'");
    }
    if (node.type != "filter" && node.type != "topic") {
      throw std::runtime_error("Unsupported node type '" + node.type + "' on node '" + node.id + "'");
    }
    if (node.type == "filter" && node.implementation != "cpp" && node.implementation != "python") {
      throw std::runtime_error(
        "Unsupported filter implementation '" + node.implementation + "' on node '" + node.id + "'");
    }
    if (node.type == "filter" && node.implementation == "cpp" && node.component_class.empty()) {
      throw std::runtime_error("Filter node '" + node.id + "' has no component class");
    }
    if (
      node.type == "filter" && node.implementation == "python" &&
      (node.python_module.empty() || node.python_class.empty()))
    {
      throw std::runtime_error("Python filter node '" + node.id + "' has no python module or class");
    }
    if (node.type == "topic" && node.topic.empty()) {
      throw std::runtime_error("Topic node '" + node.id + "' must declare a topic");
    }
    if (node.type == "topic" && node.input_type.empty() && node.output_type.empty()) {
      throw std::runtime_error("Topic node '" + node.id + "' must declare a type");
    }
    if (node.type == "topic" && !topic_names.insert(node.topic).second) {
      throw std::runtime_error("Duplicate topic name '" + node.topic + "'");
    }
    if (node.type == "filter") {
      validateSyncConfig(node);
    }
  }

  for (const auto & edge : graph.edges) {
    if (edge.from.direction != "output") {
      throw std::runtime_error("Edge from.direction must be 'output' for node '" + edge.from.node + "'");
    }
    if (edge.to.direction != "input") {
      throw std::runtime_error("Edge to.direction must be 'input' for node '" + edge.to.node + "'");
    }
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
    if (source->type == "filter" && !filterPortIsValid(*source, edge.from.port, true)) {
      throw std::runtime_error(
        "Filter output port '" + edge.from.port + "' is not declared on node '" + source->id + "'");
    }
    if (target->type == "filter" && !filterPortIsValid(*target, edge.to.port, false)) {
      throw std::runtime_error(
        "Filter input port '" + edge.to.port + "' is not declared on node '" + target->id + "'");
    }
    const auto source_type = nodeTypeForEdge(*source, true, edge.from.port);
    const auto target_type = nodeTypeForEdge(*target, false, edge.to.port);
    if (!source_type.empty() && !target_type.empty() && source_type != target_type) {
      throw std::runtime_error(
        "Type-incompatible edge from '" + edge.from.node + "' (" + source_type + ") to '" +
        edge.to.node + "' (" + target_type + ")");
    }
  }

  std::set<std::pair<std::string, std::string>> used_inputs;
  std::set<std::pair<std::string, std::string>> used_outputs;
  for (const auto & edge : graph.edges) {
    const auto source = std::find_if(
      graph.nodes.begin(),
      graph.nodes.end(),
      [&edge](const auto & node) {return node.id == edge.from.node;});
    const auto target = std::find_if(
      graph.nodes.begin(),
      graph.nodes.end(),
      [&edge](const auto & node) {return node.id == edge.to.node;});
    if (source->type == "filter") {
      const auto key = std::make_pair(
        source->id,
        canonicalPort(source->output_ports.empty() ? source->output_type : source->output_ports, edge.from.port, true));
      if (!used_outputs.insert(key).second) {
        throw std::runtime_error("Filter output '" + key.first + ":" + key.second + "' is already connected");
      }
    }
    if (target->type == "filter") {
      const auto key = std::make_pair(
        target->id,
        canonicalPort(target->input_ports.empty() ? target->input_type : target->input_ports, edge.to.port, false));
      if (!used_inputs.insert(key).second) {
        throw std::runtime_error("Filter input '" + key.first + ":" + key.second + "' is already connected");
      }
    }
  }
}

}  // namespace filter_component_factory::pipeline
