// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include "pcl_filter_components/pipeline/pipeline_factory_node.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rclcpp/parameter.hpp>
#include <rclcpp_components/node_factory.hpp>
#include <yaml-cpp/yaml.h>

namespace pcl_filter_components::pipeline
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

size_t inputIndexForPort(const std::string & port, size_t fallback)
{
  if (port.rfind("input_", 0) == 0) {
    try {
      const auto value = std::stoul(port.substr(6));
      if (value > 0U) {
        return value - 1U;
      }
    } catch (const std::exception &) {
    }
  }
  return fallback;
}

std::string inputParameterName(size_t index, size_t count)
{
  if (count <= 1U) {
    return "input_topic";
  }
  if (index == 0U) {
    return "input_topic_a";
  }
  if (index == 1U) {
    return "input_topic_b";
  }
  return "input_topic_" + std::to_string(index + 1U);
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

  const auto input_topics = inputTopicsForNode(node.id);
  for (size_t index = 0; index < input_topics.size(); ++index) {
    if (!input_topics[index].empty()) {
      parameters.push_back(rclcpp::Parameter{
          inputParameterName(index, input_topics.size()),
          input_topics[index]});
    }
  }

  const auto output_topic = outputTopicForNode(node.id);
  if (!output_topic.empty()) {
    parameters.push_back(rclcpp::Parameter{"output_topic", output_topic});
  }

  return parameters;
}

std::vector<std::string> PipelineFactoryNode::inputTopicsForNode(const std::string & node_id) const
{
  std::vector<std::string> topics;
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
      (edge.topic.empty() ? "~/" + edge.from.node + "-" + edge.to.node : edge.topic);
    const auto index = inputIndexForPort(edge.to.port, topics.size());
    if (topics.size() <= index) {
      topics.resize(index + 1U);
    }
    topics[index] = topic;
  }
  return topics;
}

std::string PipelineFactoryNode::outputTopicForNode(const std::string & node_id) const
{
  for (const auto & edge : graph_.edges) {
    if (edge.from.node != node_id) {
      continue;
    }
    const auto * target = findNode(graph_, edge.to.node);
    if (target == nullptr) {
      continue;
    }
    if (target->type == "topic") {
      return target->topic;
    }
    return edge.topic.empty() ? "~/" + edge.from.node + "-" + edge.to.node : edge.topic;
  }
  return {};
}

}  // namespace pcl_filter_components::pipeline
