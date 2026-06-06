// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include "pcl_filter_factory/pipeline/pipeline_factory_node.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rclcpp/parameter.hpp>
#include <rclcpp_components/node_factory.hpp>
#include <yaml-cpp/yaml.h>

namespace pcl_filter_factory::pipeline
{

namespace
{

const PipelineNode * findNode(const PipelineGraph & graph, const std::string & node_id)
{
  const auto iter = std::find_if(
    graph.nodes.begin(),
    graph.nodes.end(),
    [&node_id](const auto & node) {return node.id == node_id;});
  return iter == graph.nodes.end() ? nullptr : &*iter;
}

rclcpp::Parameter parameterFromString(const std::string & name, const std::string & value)
{
  const auto yaml = YAML::Load(value);
  if (yaml.IsScalar()) {
    const auto text = yaml.as<std::string>();
    if (text == "true" || text == "false") {
      return rclcpp::Parameter{name, yaml.as<bool>()};
    }
    try {
      const auto integer = yaml.as<int>();
      if (std::to_string(integer) == text) {
        return rclcpp::Parameter{name, integer};
      }
    } catch (const YAML::Exception &) {
    }
    try {
      return rclcpp::Parameter{name, yaml.as<double>()};
    } catch (const YAML::Exception &) {
    }
    return rclcpp::Parameter{name, text};
  }
  return rclcpp::Parameter{name, value};
}

std::string packageFromComponentClass(const std::string & component_class)
{
  const auto delimiter = component_class.find("::");
  if (delimiter == std::string::npos) {
    throw std::runtime_error("Component class '" + component_class + "' is not package-qualified");
  }
  return component_class.substr(0, delimiter);
}

std::string normalizedInputPort(const std::string & port)
{
  if (port == "in" || port.empty()) {
    return "cloud";
  }
  return port;
}

std::string normalizedOutputPort(const std::string & port)
{
  if (port == "out" || port.empty()) {
    return "cloud";
  }
  return port;
}

size_t inputIndexForPort(const std::string & port, size_t fallback)
{
  const auto normalized = normalizedInputPort(port);
  if (normalized.rfind("input_", 0) == 0) {
    try {
      const auto value = std::stoul(normalized.substr(6));
      if (value > 0U) {
        return value - 1U;
      }
    } catch (const std::exception &) {
    }
  }
  return fallback;
}

std::string inputParameterName(const std::string & port)
{
  return "inputs." + normalizedInputPort(port) + ".topic";
}

std::string outputParameterName(const std::string & port)
{
  return "outputs." + normalizedOutputPort(port) + ".topic";
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
  return name.empty() ? (outgoing ? "out" : "in") : name;
}

std::vector<std::string> portsForType(const std::string & value, bool outgoing)
{
  const auto ports_with_types = splitPorts(value);
  auto ports = std::vector<std::string>{};
  for (size_t index = 0; index < ports_with_types.size(); ++index) {
    const auto & port_name = ports_with_types[index].first;
    const auto & stream_type = ports_with_types[index].second;
    ports.push_back(port_name.empty() ?
      portNameForType(stream_type, index, ports_with_types.size(), outgoing) :
      port_name);
  }
  return ports;
}

std::string topicNamePartForText(const std::string & text)
{
  auto name = text;
  if (name.rfind("~/", 0) == 0) {
    name.erase(0, 2);
  }
  while (!name.empty() && name.front() == '/') {
    name.erase(name.begin());
  }
  while (!name.empty() && name.back() == '/') {
    name.pop_back();
  }
  std::replace(name.begin(), name.end(), '/', '_');
  std::replace(name.begin(), name.end(), '-', '_');
  return name.empty() ? "node" : name;
}

std::string sharedEdgeTopic(const PipelineEdge & edge)
{
  return "/" + topicNamePartForText(edge.from.node) + "_" + topicNamePartForText(edge.to.node);
}

std::string hiddenPortTopic(const std::string & direction, const std::string & port)
{
  return "~/_" + direction + "/" + topicNamePartForText(port);
}

void appendPortQosParameters(
  std::vector<rclcpp::Parameter> & parameters,
  const std::string & direction,
  const std::map<std::string, std::map<std::string, std::string>> & ports)
{
  for (const auto & [port, qos] : ports) {
    const auto normalized_port = direction == "inputs" ? normalizedInputPort(port) : normalizedOutputPort(port);
    for (const auto & [name, value] : qos) {
      parameters.push_back(parameterFromString(direction + "." + normalized_port + ".qos." + name, value));
    }
  }
}

}  // namespace

PipelineFactoryNode::PipelineFactoryNode(
  std::weak_ptr<rclcpp::Executor> executor,
  const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("pcl_pipeline_factory", options),
  executor_(std::move(executor))
{
  this->declare_parameter<std::string>("pipeline_file", "");
  this->declare_parameter<int>("executor_threads", 0);
  auto manager_options = options;
  manager_options.use_intra_process_comms(true);
  manager_options.start_parameter_services(false);
  manager_options.start_parameter_event_publisher(false);
  component_manager_ = std::make_shared<rclcpp_components::ComponentManager>(
    executor_,
    "pcl_pipeline_component_manager",
    manager_options);
}

PipelineFactoryNode::CallbackReturn PipelineFactoryNode::on_configure(
  const rclcpp_lifecycle::State & previous_state)
{
  (void)previous_state;
  pipeline_file_ = this->get_parameter("pipeline_file").as_string();
  if (pipeline_file_.empty()) {
    RCLCPP_ERROR(this->get_logger(), "pipeline_file parameter is empty");
    return CallbackReturn::FAILURE;
  }

  try {
    graph_ = loadPipelineGraph(pipeline_file_);
    loadPipeline();
  } catch (const std::exception & error) {
    RCLCPP_ERROR(this->get_logger(), "Failed to load pipeline '%s': %s", pipeline_file_.c_str(), error.what());
    unloadPipeline();
    return CallbackReturn::FAILURE;
  }

  return CallbackReturn::SUCCESS;
}

PipelineFactoryNode::CallbackReturn PipelineFactoryNode::on_activate(
  const rclcpp_lifecycle::State & previous_state)
{
  (void)previous_state;
  for (auto & loaded : loaded_components_) {
    auto lifecycle_node = std::static_pointer_cast<rclcpp_lifecycle::LifecycleNode>(
      loaded.wrapper.get_node_instance());
    CallbackReturn callback_return;
    lifecycle_node->activate(callback_return);
    if (callback_return != CallbackReturn::SUCCESS) {
      RCLCPP_ERROR(this->get_logger(), "Failed to activate component '%s'", loaded.id.c_str());
      return CallbackReturn::FAILURE;
    }
  }
  return CallbackReturn::SUCCESS;
}

PipelineFactoryNode::CallbackReturn PipelineFactoryNode::on_deactivate(
  const rclcpp_lifecycle::State & previous_state)
{
  (void)previous_state;
  for (auto & loaded : loaded_components_) {
    auto lifecycle_node = std::static_pointer_cast<rclcpp_lifecycle::LifecycleNode>(
      loaded.wrapper.get_node_instance());
    CallbackReturn callback_return;
    lifecycle_node->deactivate(callback_return);
  }
  return CallbackReturn::SUCCESS;
}

PipelineFactoryNode::CallbackReturn PipelineFactoryNode::on_cleanup(
  const rclcpp_lifecycle::State & previous_state)
{
  (void)previous_state;
  for (auto & loaded : loaded_components_) {
    auto lifecycle_node = std::static_pointer_cast<rclcpp_lifecycle::LifecycleNode>(
      loaded.wrapper.get_node_instance());
    CallbackReturn callback_return;
    lifecycle_node->cleanup(callback_return);
  }
  unloadPipeline();
  graph_ = PipelineGraph{};
  return CallbackReturn::SUCCESS;
}

void PipelineFactoryNode::loadPipeline()
{
  auto executor = executor_.lock();
  if (!executor) {
    throw std::runtime_error("Pipeline factory requires a live executor");
  }

  for (const auto & node : graph_.nodes) {
    if (node.type != "filter") {
      continue;
    }

    const auto package_name = packageFromComponentClass(node.component_class);
    const auto resources = component_manager_->get_component_resources(package_name);
    const auto resource = std::find_if(
      resources.begin(),
      resources.end(),
      [&node](const auto & item) {return item.first == node.component_class;});
    if (resource == resources.end()) {
      throw std::runtime_error("Component '" + node.component_class + "' is not registered");
    }

    auto factory = component_manager_->create_component_factory(*resource);
    auto options = this->get_node_options();
    options.use_intra_process_comms(true);
    options.arguments({"--ros-args", "-r", "__node:=" + node.id});
    options.parameter_overrides(parametersForNode(node));
    auto wrapper = factory->create_node_instance(options);
    executor->add_node(wrapper.get_node_base_interface());

    auto lifecycle_node = std::static_pointer_cast<rclcpp_lifecycle::LifecycleNode>(
      wrapper.get_node_instance());
    CallbackReturn callback_return;
    lifecycle_node->configure(callback_return);
    if (callback_return != CallbackReturn::SUCCESS) {
      executor->remove_node(wrapper.get_node_base_interface());
      throw std::runtime_error("Failed to configure component '" + node.id + "'");
    }

    loaded_components_.push_back({node.id, std::move(wrapper)});
  }
}

void PipelineFactoryNode::unloadPipeline()
{
  auto executor = executor_.lock();
  for (auto & loaded : loaded_components_) {
    if (executor) {
      executor->remove_node(loaded.wrapper.get_node_base_interface());
    }
  }
  loaded_components_.clear();
}

std::vector<rclcpp::Parameter> PipelineFactoryNode::parametersForNode(const PipelineNode & node) const
{
  auto parameters = std::vector<rclcpp::Parameter>{};
  for (const auto & [name, value] : node.parameters) {
    parameters.push_back(parameterFromString(name, value));
  }

  const auto inbound_topics = inputTopicsForNode(node.id);
  for (const auto & [port, topic] : inbound_topics) {
    if (!topic.empty()) {
      parameters.push_back(rclcpp::Parameter{
          inputParameterName(port),
          topic});
    }
  }
  for (const auto & port : portsForType(node.input_ports.empty() ? node.input_type : node.input_ports, false)) {
    const auto existing = std::find_if(
      inbound_topics.begin(),
      inbound_topics.end(),
      [&port](const auto & item) {return item.first == port;});
    if (existing != inbound_topics.end()) {
      continue;
    }
    const auto topic = hiddenPortTopic("input", port);
    RCLCPP_WARN(
      this->get_logger(),
      "Filter '%s' input port '%s' has no configured topic; using hidden fallback '%s'",
      node.id.c_str(),
      port.c_str(),
      topic.c_str());
    parameters.push_back(rclcpp::Parameter{inputParameterName(port), topic});
  }

  const auto outbound_topics = outputTopicsForNode(node.id);
  for (const auto & [port, topic] : outbound_topics) {
    if (!topic.empty()) {
      parameters.push_back(rclcpp::Parameter{outputParameterName(port), topic});
    }
  }
  for (const auto & port : portsForType(node.output_ports.empty() ? node.output_type : node.output_ports, true)) {
    const auto existing = std::find_if(
      outbound_topics.begin(),
      outbound_topics.end(),
      [&port](const auto & item) {return item.first == port;});
    if (existing != outbound_topics.end()) {
      continue;
    }
    const auto topic = hiddenPortTopic("output", port);
    RCLCPP_WARN(
      this->get_logger(),
      "Filter '%s' output port '%s' has no configured topic; using hidden fallback '%s'",
      node.id.c_str(),
      port.c_str(),
      topic.c_str());
    parameters.push_back(rclcpp::Parameter{outputParameterName(port), topic});
  }

  appendPortQosParameters(parameters, "inputs", node.inputs);
  appendPortQosParameters(parameters, "outputs", node.outputs);

  for (const auto & [name, value] : node.sync) {
    parameters.push_back(parameterFromString("sync." + name, value));
  }

  return parameters;
}

std::vector<std::pair<std::string, std::string>> PipelineFactoryNode::inputTopicsForNode(
  const std::string & node_id) const
{
  std::vector<std::pair<std::string, std::string>> topics;
  for (const auto & edge : graph_.edges) {
    if (edge.to.node != node_id) {
      continue;
    }
    const auto * source = findNode(graph_, edge.from.node);
    if (source == nullptr) {
      continue;
    }
    const auto topic = source->type == "topic" ?
      source->topic :
      (edge.topic.empty() ? sharedEdgeTopic(edge) : edge.topic);
    const auto index = inputIndexForPort(edge.to.port, topics.size());
    if (topics.size() <= index) {
      topics.resize(index + 1U);
    }
    topics[index] = {normalizedInputPort(edge.to.port), topic};
  }
  return topics;
}

std::vector<std::pair<std::string, std::string>> PipelineFactoryNode::outputTopicsForNode(
  const std::string & node_id) const
{
  std::vector<std::pair<std::string, std::string>> topics;
  for (const auto & edge : graph_.edges) {
    if (edge.from.node != node_id) {
      continue;
    }
    const auto * target = findNode(graph_, edge.to.node);
    if (target == nullptr) {
      continue;
    }
    if (target->type == "topic") {
      topics.push_back({normalizedOutputPort(edge.from.port), target->topic});
      continue;
    }
    topics.push_back(
      {normalizedOutputPort(edge.from.port),
        edge.topic.empty() ? sharedEdgeTopic(edge) : edge.topic});
  }
  return topics;
}

}  // namespace pcl_filter_factory::pipeline
