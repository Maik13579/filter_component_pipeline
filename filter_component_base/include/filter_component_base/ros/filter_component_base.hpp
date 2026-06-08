// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_
#define FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_

#include <memory>
#include <array>
#include <functional>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rclcpp/create_publisher.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "filter_component_base/ros/parameter_utils.hpp"
#include "filter_component_synchronizer/filter_component_synchronizer.hpp"

namespace filter_component_base::ros
{

class FilterComponentBase : public rclcpp_lifecycle::LifecycleNode
{
public:
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

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

  struct PortDescriptor
  {
    std::string name;
    std::string description;
    std::string default_reliability{"best_effort"};
    std::string default_history{"keep_last"};
    int default_depth{5};
    std::string default_durability{"volatile"};
    std::type_index adapter_type;
    std::function<std::unique_ptr<PublisherConcept>(
        FilterComponentBase &,
        const std::string &,
        const rclcpp::QoS &)> create_publisher;
    std::function<void(
        filter_component_synchronizer::FilterComponentSynchronizer &,
        FilterComponentBase &,
        const std::string &,
        const std::string &,
        const rclcpp::QoS &)> create_subscription;
  };

  template <typename AdapterT>
  static PortDescriptor inputPort(
    const std::string & name,
    const std::string & description)
  {
    return {
      name,
      description,
      "best_effort",
      "keep_last",
      5,
      "volatile",
      std::type_index(typeid(AdapterT)),
      {},
      [](
        filter_component_synchronizer::FilterComponentSynchronizer & synchronizer,
        FilterComponentBase & node,
        const std::string & port_name,
        const std::string & topic_name,
        const rclcpp::QoS & qos)
      {
        synchronizer.template addInput<FilterComponentBase, AdapterT>(
          node,
          port_name,
          topic_name,
          qos);
      }};
  }

  template <typename AdapterT>
  static PortDescriptor outputPort(
    const std::string & name,
    const std::string & description)
  {
    return {
      name,
      description,
      "best_effort",
      "keep_last",
      5,
      "volatile",
      std::type_index(typeid(AdapterT)),
      [](
        FilterComponentBase & node,
        const std::string & topic_name,
        const rclcpp::QoS & qos)
      {
        return std::make_unique<PublisherHolder<AdapterT>>(
          node.template createAdaptedPublisher<AdapterT>(topic_name, qos));
      },
      {}};
  }

  template <size_t InputCount, size_t OutputCount>
  FilterComponentBase(
    const std::string & node_name,
    const rclcpp::NodeOptions & options,
    const std::array<PortDescriptor, InputCount> & input_ports,
    const std::array<PortDescriptor, OutputCount> & output_ports)
  : rclcpp_lifecycle::LifecycleNode(node_name, options)
  {
    input_ports_.assign(input_ports.begin(), input_ports.end());
    output_ports_.assign(output_ports.begin(), output_ports.end());
    for (const auto & port : input_ports_) {
      declareParameterIfNotDeclared(
        *this,
        inputTopicParameterName(port.name),
        defaultPortTopic("input", port.name),
        makeParameterDescriptor(port.description));
      declarePortQosParameters("inputs", port);
    }
    for (const auto & port : output_ports_) {
      declareParameterIfNotDeclared(
        *this,
        outputTopicParameterName(port.name),
        defaultPortTopic("output", port.name),
        makeParameterDescriptor(port.description));
      declarePortQosParameters("outputs", port);
    }
    if (input_ports_.size() > 1U) {
      declareParameterIfNotDeclared(
        *this,
        "sync.mode",
        std::string{"receipt_time"},
        makeParameterDescriptor(
          "Input synchronization mode.",
          "Supported values: receipt_time, latest."));
      declareParameterIfNotDeclared(
        *this,
        "sync.queue_size",
        10,
        makeIntegerRangeParameterDescriptor("Maximum unmatched input messages kept per port.", 1, 100000));
      declareParameterIfNotDeclared(
        *this,
        "sync.max_interval",
        0.05,
        makeFloatingPointRangeParameterDescriptor(
          "Maximum receipt-time span across a synchronized input set in seconds.",
          0.0,
          3600.0));
    }
  }

protected:
  static std::string inputTopicParameterName(const std::string & port_name)
  {
    return "inputs." + port_name + ".topic";
  }

  static std::string outputTopicParameterName(const std::string & port_name)
  {
    return "outputs." + port_name + ".topic";
  }

  static std::string portQosParameterName(
    const std::string & direction,
    const std::string & port_name,
    const std::string & field)
  {
    return direction + "." + port_name + ".qos." + field;
  }

  static std::string defaultPortTopic(const std::string & direction, const std::string & port_name)
  {
    return "~/_" + direction + "/" + port_name;
  }

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;

    try {
      readPortTopics();

      configure();

      active_ = false;
      configureInterfaces();
    } catch (const std::exception & exception) {
      cleanupInterfaces();
      active_ = false;
      RCLCPP_ERROR(get_logger(), "Failed to configure filter component: %s", exception.what());
      return CallbackReturn::FAILURE;
    }

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
    active_ = false;
    return CallbackReturn::SUCCESS;
  }

  virtual void configure() = 0;

  virtual void configureInterfaces()
  {
    for (const auto & port : output_ports_) {
      if (!port.create_publisher) {
        throw std::runtime_error("Output port '" + port.name + "' has no publisher factory");
      }
      publishers_.emplace(
        port.name,
        port.create_publisher(*this, outbound_topics_.at(port.name), portQos("outputs", port.name)));
    }

    auto sync_options = filter_component_synchronizer::SynchronizerOptions{};
    if (input_ports_.size() > 1U) {
      sync_options.mode = filter_component_synchronizer::syncModeFromString(
        getParameter<std::string>(*this, "sync.mode"));
      const auto sync_queue_size = getParameter<int>(*this, "sync.queue_size");
      sync_options.queue_size = sync_queue_size > 0 ?
        static_cast<size_t>(sync_queue_size) : 1U;
      sync_options.max_interval = getParameter<double>(*this, "sync.max_interval");
    }
    synchronizer_ = std::make_unique<filter_component_synchronizer::FilterComponentSynchronizer>(
      sync_options,
      std::bind(&FilterComponentBase::processSynchronizedInputs, this));
    for (const auto & port : input_ports_) {
      if (!port.create_subscription) {
        throw std::runtime_error("Input port '" + port.name + "' has no subscription factory");
      }
      port.create_subscription(
        *synchronizer_,
        *this,
        port.name,
        inbound_topics_.at(port.name),
        portQos("inputs", port.name));
    }
  }

  virtual void cleanupInterfaces()
  {
    synchronizer_.reset();
    publishers_.clear();
  }

  virtual void process() = 0;

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

  void processSynchronizedInputs()
  {
    if (
      !active_ ||
      this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
    {
      return;
    }
    process();
  }

  std::vector<PortDescriptor> input_ports_;
  std::vector<PortDescriptor> output_ports_;
  std::unordered_map<std::string, std::string> inbound_topics_;
  std::unordered_map<std::string, std::string> outbound_topics_;

  void readPortTopics()
  {
    inbound_topics_.clear();
    outbound_topics_.clear();
    for (const auto & port : input_ports_) {
      inbound_topics_[port.name] =
        getParameter<std::string>(*this, inputTopicParameterName(port.name));
    }
    for (const auto & port : output_ports_) {
      outbound_topics_[port.name] =
        getParameter<std::string>(*this, outputTopicParameterName(port.name));
    }
  }

  void declarePortQosParameters(const std::string & direction, const PortDescriptor & port)
  {
    declareParameterIfNotDeclared(
      *this,
      portQosParameterName(direction, port.name, "reliability"),
      port.default_reliability,
      makeParameterDescriptor(
        "QoS reliability for port '" + port.name + "'.",
        "Supported values: best_effort, reliable."));
    declareParameterIfNotDeclared(
      *this,
      portQosParameterName(direction, port.name, "history"),
      port.default_history,
      makeParameterDescriptor(
        "QoS history policy for port '" + port.name + "'.",
        "Supported values: keep_last, keep_all."));
    declareParameterIfNotDeclared(
      *this,
      portQosParameterName(direction, port.name, "depth"),
      port.default_depth,
      makeIntegerRangeParameterDescriptor(
        "QoS history depth for port '" + port.name + "'.",
        1,
        100000));
    declareParameterIfNotDeclared(
      *this,
      portQosParameterName(direction, port.name, "durability"),
      port.default_durability,
      makeParameterDescriptor(
        "QoS durability for port '" + port.name + "'.",
        "Supported values: volatile, transient_local."));
  }

  rclcpp::QoS portQos(const std::string & direction, const std::string & port_name)
  {
    const auto reliability = getParameter<std::string>(
      *this,
      portQosParameterName(direction, port_name, "reliability"));
    const auto history = getParameter<std::string>(
      *this,
      portQosParameterName(direction, port_name, "history"));
    const auto depth_value = getParameter<int>(
      *this,
      portQosParameterName(direction, port_name, "depth"));
    const auto durability = getParameter<std::string>(
      *this,
      portQosParameterName(direction, port_name, "durability"));

    auto qos = rclcpp::QoS(rclcpp::KeepLast(depth_value > 0 ? static_cast<size_t>(depth_value) : 1U));
    if (history == "keep_all") {
      qos.keep_all();
    } else if (history == "keep_last") {
      qos.keep_last(depth_value > 0 ? static_cast<size_t>(depth_value) : 1U);
    } else {
      throw std::runtime_error("Unsupported QoS history '" + history + "' for " + direction + "." + port_name);
    }

    if (reliability == "best_effort") {
      qos.best_effort();
    } else if (reliability == "reliable") {
      qos.reliable();
    } else {
      throw std::runtime_error(
        "Unsupported QoS reliability '" + reliability + "' for " + direction + "." + port_name);
    }

    if (durability == "volatile") {
      qos.durability_volatile();
    } else if (durability == "transient_local") {
      qos.transient_local();
    } else {
      throw std::runtime_error(
        "Unsupported QoS durability '" + durability + "' for " + direction + "." + port_name);
    }
    return qos;
  }

  bool active_{false};
  std::unique_ptr<filter_component_synchronizer::FilterComponentSynchronizer> synchronizer_;
  std::unordered_map<std::string, std::unique_ptr<PublisherConcept>> publishers_;
};

}  // namespace filter_component_base::ros

#endif  // FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_
