// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_BASE__ROS__PCL_FILTER_COMPONENT_BASE_HPP_
#define PCL_FILTER_BASE__ROS__PCL_FILTER_COMPONENT_BASE_HPP_

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rclcpp/create_publisher.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "pcl_filter_base/ros/parameter_utils.hpp"
#include "pcl_filter_synchronizer/pcl_filter_synchronizer.hpp"
#include "pcl_filter_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_base::ros
{

template <typename PointT, typename FilterT>
class PclFilterComponentBase : public rclcpp_lifecycle::LifecycleNode
{
public:
  struct PortDescriptor
  {
    std::string name;
    std::string default_topic;
    std::string description;
  };

  using StampedCloud = pcl::PointCloud<PointT>;
  using Cloud = StampedCloud;
  using CloudAdapter = pcl_filter_type_adapters::ros::PclCloudAdapter<PointT>;
  using Publisher = rclcpp::Publisher<CloudAdapter>;
  using Subscription = rclcpp::Subscription<CloudAdapter>;
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  explicit PclFilterComponentBase(
    const std::string & node_name,
    const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : PclFilterComponentBase(node_name, options, defaultInputPorts(), defaultOutputPorts())
  {
  }

  PclFilterComponentBase(
    const std::string & node_name,
    const rclcpp::NodeOptions & options,
    std::vector<PortDescriptor> input_ports,
    std::vector<PortDescriptor> output_ports)
  : rclcpp_lifecycle::LifecycleNode(node_name, options)
  {
    input_ports_ = std::move(input_ports);
    output_ports_ = std::move(output_ports);
    for (const auto & port : input_ports_) {
      declareParameterIfNotDeclared(
        *this,
        inputTopicParameterName(port.name),
        port.default_topic,
        makeParameterDescriptor(port.description));
    }
    for (const auto & port : output_ports_) {
      declareParameterIfNotDeclared(
        *this,
        outputTopicParameterName(port.name),
        port.default_topic,
        makeParameterDescriptor(port.description));
    }
    declareParameterIfNotDeclared(
      *this,
      "input_topic",
      std::string{"/points/input"},
      makeParameterDescriptor("Compatibility alias for inputs.cloud.topic."));
    declareParameterIfNotDeclared(
      *this,
      "output_topic",
      std::string{"/points/output"},
      makeParameterDescriptor("Compatibility alias for outputs.cloud.topic."));
    declareParameterIfNotDeclared(
      *this,
      "queue_size",
      5,
      makeIntegerRangeParameterDescriptor(
        "Depth used for input subscriptions and output publishers.",
        1,
        100000));
    if (input_ports_.size() > 1U) {
      declareParameterIfNotDeclared(
        *this,
        "sync.policy",
        std::string{"ExactTime"},
        makeParameterDescriptor(
          "Input synchronization policy.",
          "Supported values: ExactTime, ApproximateTime."));
      declareParameterIfNotDeclared(
        *this,
        "sync.queue_size",
        10,
        makeIntegerRangeParameterDescriptor("Maximum unmatched input messages kept per port.", 1, 100000));
      declareParameterIfNotDeclared(
        *this,
        "sync.slop",
        0.05,
        makeFloatingPointRangeParameterDescriptor(
          "Maximum timestamp difference for ApproximateTime synchronization in seconds.",
          0.0,
          3600.0));
    }
  }

protected:
  static std::vector<PortDescriptor> defaultInputPorts()
  {
    return {{"cloud", "/points/input", "Input point cloud topic subscribed by the filter."}};
  }

  static std::vector<PortDescriptor> defaultOutputPorts()
  {
    return {
      {"cloud", "/points/output", "Output point cloud topic published by the filter."},
      {"indices", "/points/output", "Output point indices topic published by the filter."},
    };
  }

  static std::string inputTopicParameterName(const std::string & port_name)
  {
    return "inputs." + port_name + ".topic";
  }

  static std::string outputTopicParameterName(const std::string & port_name)
  {
    return "outputs." + port_name + ".topic";
  }

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;

    queue_size_ = getParameter<int>(*this, "queue_size");
    readPortTopics();

    filter_ = FilterT{};
    configureFilter();

    const auto depth = queue_size_ > 0 ? static_cast<size_t>(queue_size_) : 1U;
    const auto qos = rclcpp::SensorDataQoS().keep_last(depth);

    active_ = false;
    configureInterfaces(qos);

    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;
    active_ = true;
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;
    active_ = false;
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;
    cleanupInterfaces();
    filter_ = FilterT{};
    active_ = false;
    return CallbackReturn::SUCCESS;
  }

  virtual void configureFilter() = 0;

  virtual void configureInterfaces(const rclcpp::QoS & qos)
  {
    for (const auto & port : output_ports_) {
      publishers_.emplace(
        port.name,
        std::make_unique<PublisherHolder<CloudAdapter>>(
          createAdaptedPublisher<CloudAdapter>(output_topics_.at(port.name), qos)));
    }

    auto sync_options = pcl_filter_synchronizer::SynchronizerOptions{};
    if (input_ports_.size() > 1U) {
      sync_options.policy = pcl_filter_synchronizer::syncPolicyFromString(
        getParameter<std::string>(*this, "sync.policy"));
      const auto sync_queue_size = getParameter<int>(*this, "sync.queue_size");
      sync_options.queue_size = sync_queue_size > 0 ?
        static_cast<size_t>(sync_queue_size) : 1U;
      sync_options.slop = getParameter<double>(*this, "sync.slop");
    }
    synchronizer_ = std::make_unique<pcl_filter_synchronizer::PclFilterSynchronizer>(
      sync_options,
      std::bind(&PclFilterComponentBase::processSynchronizedInputs, this));
    for (const auto & port : input_ports_) {
      synchronizer_->template addInput<rclcpp_lifecycle::LifecycleNode, CloudAdapter>(
        *this,
        port.name,
        input_topics_.at(port.name),
        qos);
    }
  }

  virtual void cleanupInterfaces()
  {
    synchronizer_.reset();
    publishers_.clear();
  }

  virtual void processCloud(std::unique_ptr<StampedCloud> input)
  {
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    filter_.filter(*input, *output);
    publishCloud(std::move(output));
  }

  template <typename AdapterT>
  std::shared_ptr<rclcpp::Publisher<AdapterT>> createAdaptedPublisher(
    const std::string & topic_name,
    const rclcpp::QoS & qos)
  {
    return rclcpp::create_publisher<AdapterT>(*this, topic_name, qos);
  }

  template <typename AdapterT>
  std::unique_ptr<typename rclcpp::TypeAdapter<AdapterT>::custom_type> takeInput(
    const std::string & port_name)
  {
    if (!synchronizer_) {
      throw std::runtime_error("Cannot take input before interfaces are configured");
    }
    return synchronizer_->template takeInput<AdapterT>(port_name);
  }

  template <typename AdapterT>
  const typename rclcpp::TypeAdapter<AdapterT>::custom_type * peekInput(
    const std::string & port_name) const
  {
    if (!synchronizer_) {
      throw std::runtime_error("Cannot peek input before interfaces are configured");
    }
    return synchronizer_->template peekInput<AdapterT>(port_name);
  }

  template <typename AdapterT>
  void publish(
    const std::string & port_name,
    std::unique_ptr<typename rclcpp::TypeAdapter<AdapterT>::custom_type> message)
  {
    auto iter = publishers_.find(port_name);
    if (iter == publishers_.end()) {
      throw std::out_of_range("Unknown output port '" + port_name + "'");
    }
    if (iter->second->adapter_type != std::type_index(typeid(AdapterT))) {
      throw std::invalid_argument("Output port '" + port_name + "' has a different adapter type");
    }
    static_cast<PublisherHolder<AdapterT> *>(iter->second.get())->publisher->publish(std::move(message));
  }

  void publishCloud(std::unique_ptr<StampedCloud> output)
  {
    publish<CloudAdapter>("cloud", std::move(output));
  }

  virtual void processInputs()
  {
    processCloud(takeInput<CloudAdapter>("cloud"));
  }

  void processSynchronizedInputs()
  {
    if (
      !active_ ||
      this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
    {
      return;
    }
    processInputs();
  }

  FilterT filter_;
  std::vector<PortDescriptor> input_ports_;
  std::vector<PortDescriptor> output_ports_;
  std::unordered_map<std::string, std::string> input_topics_;
  std::unordered_map<std::string, std::string> output_topics_;
  int queue_size_{5};

  struct PublisherConcept
  {
    explicit PublisherConcept(std::type_index adapter_type_in)
    : adapter_type(adapter_type_in)
    {
    }

    virtual ~PublisherConcept() = default;
    std::type_index adapter_type;
  };

  template <typename AdapterT>
  struct PublisherHolder : PublisherConcept
  {
    explicit PublisherHolder(std::shared_ptr<rclcpp::Publisher<AdapterT>> publisher_in)
    : PublisherConcept(std::type_index(typeid(AdapterT))),
      publisher(std::move(publisher_in))
    {
    }

    std::shared_ptr<rclcpp::Publisher<AdapterT>> publisher;
  };

  void readPortTopics()
  {
    input_topics_.clear();
    output_topics_.clear();
    for (const auto & port : input_ports_) {
      auto topic = getParameter<std::string>(*this, inputTopicParameterName(port.name));
      if (port.name == "cloud" && topic == port.default_topic && this->has_parameter("input_topic")) {
        topic = getParameter<std::string>(*this, "input_topic");
      }
      input_topics_[port.name] = topic;
    }
    for (const auto & port : output_ports_) {
      auto topic = getParameter<std::string>(*this, outputTopicParameterName(port.name));
      if (port.name == "cloud" && topic == port.default_topic && this->has_parameter("output_topic")) {
        topic = getParameter<std::string>(*this, "output_topic");
      }
      output_topics_[port.name] = topic;
    }
  }

  bool active_{false};
  std::unique_ptr<pcl_filter_synchronizer::PclFilterSynchronizer> synchronizer_;
  std::unordered_map<std::string, std::unique_ptr<PublisherConcept>> publishers_;
};

}  // namespace pcl_filter_base::ros

#endif  // PCL_FILTER_BASE__ROS__PCL_FILTER_COMPONENT_BASE_HPP_
