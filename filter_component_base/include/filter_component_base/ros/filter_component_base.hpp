// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_
#define FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <component_shm/shm_view.hpp>
#include <rcl_interfaces/msg/floating_point_range.hpp>
#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/create_publisher.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "filter_component_base/ros/component_descriptors.hpp"
#include "filter_component_synchronizer/filter_component_synchronizer.hpp"

namespace filter_component_base::ros
{

class FilterComponentBase : public rclcpp_lifecycle::LifecycleNode
{
public:
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
  using PortDescriptor = filter_component_base::ros::PortDescriptor;
  using PublisherConcept = filter_component_base::ros::PublisherConcept;
  using ShmAccess = filter_component_base::ros::ShmAccess;
  using ShmKeyDescriptor = filter_component_base::ros::ShmKeyDescriptor;

  /// @brief Declare a parameter with a default value and description.
  template <typename T>
  void declareParameter(
    const std::string & name,
    const T & default_value,
    const std::string & description)
  {
    this->declare_parameter<T>(name, default_value, makeParameterDescriptor(description));
  }

  /// @brief Read a declared parameter as a typed value.
  template <typename T>
  T getParameter(const std::string & name) const
  {
    return this->get_parameter(name).get_value<T>();
  }

  /// @brief Create a shared-memory key descriptor.
  template<typename T>
  static ShmKeyDescriptor shmKey(
    const std::string & name,
    const std::string & description,
    ShmAccess access,
    const std::string & type_name = typeid(T).name())
  {
    using StoredType = std::remove_cv_t<T>;
    return {
      name,
      description,
      type_name,
      std::type_index(typeid(StoredType)),
      access};
  }

  /// @brief Create an input topic port descriptor.
  template <typename AdapterT>
  static PortDescriptor inputPort(
    const std::string & name,
    const std::string & description,
    const std::string & type_name = typeid(AdapterT).name())
  {
    return {
      name,
      description,
      type_name,
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

  /// @brief Create an output topic port descriptor.
  template <typename AdapterT>
  static PortDescriptor outputPort(
    const std::string & name,
    const std::string & description,
    const std::string & type_name = typeid(AdapterT).name())
  {
    return {
      name,
      description,
      type_name,
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
        return std::make_unique<filter_component_base::ros::PublisherHolder<AdapterT>>(
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
  : FilterComponentBase(
      node_name,
      options,
      input_ports,
      output_ports,
      std::array<ShmKeyDescriptor, 0U>{})
  {
  }

  template <size_t InputCount, size_t OutputCount, size_t ShmKeyCount>
  FilterComponentBase(
    const std::string & node_name,
    const rclcpp::NodeOptions & options,
    const std::array<PortDescriptor, InputCount> & input_ports,
    const std::array<PortDescriptor, OutputCount> & output_ports,
    const std::array<ShmKeyDescriptor, ShmKeyCount> & shm_keys)
  : rclcpp_lifecycle::LifecycleNode(node_name, options)
  {
    input_ports_.assign(input_ports.begin(), input_ports.end());
    output_ports_.assign(output_ports.begin(), output_ports.end());
    shm_keys_.assign(shm_keys.begin(), shm_keys.end());
    for (const auto & port : input_ports_) {
      declareParameter(
        inputTopicParameterName(port.name),
        defaultPortTopic("input", port.name),
        port.description);
      declarePortQosParameters("inputs", port);
    }
    for (const auto & port : output_ports_) {
      declareParameter(
        outputTopicParameterName(port.name),
        defaultPortTopic("output", port.name),
        port.description);
      declarePortQosParameters("outputs", port);
    }
    for (const auto & key : shm_keys_) {
      declareParameter(
        shmKeyParameterName(key.name),
        key.name,
        key.description);
    }
    if (input_ports_.size() > 1U) {
      declareParameter(
        "sync.mode",
        std::string{"receipt_time"},
        "Input synchronization mode.");
      declareParameterWithDescriptor(
        "sync.queue_size",
        10,
        makeIntegerRangeParameterDescriptor("Maximum unmatched input messages kept per port.", 1, 100000));
      declareParameterWithDescriptor(
        "sync.max_interval",
        0.05,
        makeFloatingPointRangeParameterDescriptor(
          "Maximum receipt-time span across a synchronized input set in seconds.",
          0.0,
          3600.0));
    }
  }

  const std::vector<PortDescriptor> & inputPortDescriptors() const
  {
    return input_ports_;
  }

  const std::vector<PortDescriptor> & outputPortDescriptors() const
  {
    return output_ports_;
  }

  const std::vector<ShmKeyDescriptor> & shmKeyDescriptors() const
  {
    return shm_keys_;
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

  static std::string shmKeyParameterName(const std::string & key_name)
  {
    return "shm_key." + key_name;
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
      configureShmView();

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
        getParameter<std::string>("sync.mode"));
      const auto sync_queue_size = getParameter<int>("sync.queue_size");
      sync_options.queue_size = sync_queue_size > 0 ?
        static_cast<size_t>(sync_queue_size) : 1U;
      sync_options.max_interval = getParameter<double>("sync.max_interval");
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

  template<typename T>
  std::shared_ptr<const std::remove_cv_t<T>> shmGet(const std::string & key) const
  {
    verifyShmKey<T>(key);
    return checkedShmView().template get<const std::remove_cv_t<T>>(key);
  }

  template<typename T>
  std::shared_ptr<const std::remove_cv_t<T>> shmTryGet(const std::string & key) const
  {
    verifyShmKey<T>(key);
    return checkedShmView().template tryGet<const std::remove_cv_t<T>>(key);
  }

  template<typename T>
  std::shared_ptr<std::remove_cv_t<T>> shmGetMutable(const std::string & key) const
  {
    verifyWritableShmKey<T>(key);
    return checkedShmView().template get<std::remove_cv_t<T>>(key);
  }

  template<typename T>
  std::shared_ptr<std::remove_cv_t<T>> shmTryGetMutable(const std::string & key) const
  {
    verifyWritableShmKey<T>(key);
    return checkedShmView().template tryGet<std::remove_cv_t<T>>(key);
  }

  template<typename T>
  void shmSet(const std::string & key, T value)
  {
    using StoredType = std::decay_t<T>;
    verifyWritableShmKey<StoredType>(key);
    checkedShmView().template set<StoredType>(key, std::move(value));
  }

  template<typename T>
  void shmSetShared(const std::string & key, std::shared_ptr<T> value)
  {
    using StoredType = std::remove_cv_t<T>;
    verifyWritableShmKey<StoredType>(key);
    checkedShmView().template setShared<StoredType>(key, std::move(value));
  }

  const component_shm::ShmView & shmView() const
  {
    return checkedShmView();
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
    static_cast<filter_component_base::ros::PublisherHolder<AdapterT> *>(
      iter->second.get())->publisher->publish(std::move(message));
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
  std::vector<ShmKeyDescriptor> shm_keys_;
  std::unordered_map<std::string, std::string> inbound_topics_;
  std::unordered_map<std::string, std::string> outbound_topics_;

  void readPortTopics()
  {
    inbound_topics_.clear();
    outbound_topics_.clear();
    for (const auto & port : input_ports_) {
      inbound_topics_[port.name] =
        getParameter<std::string>(inputTopicParameterName(port.name));
    }
    for (const auto & port : output_ports_) {
      outbound_topics_[port.name] =
        getParameter<std::string>(outputTopicParameterName(port.name));
    }
  }

  void declarePortQosParameters(const std::string & direction, const PortDescriptor & port)
  {
    declareParameterWithDescriptor(
      portQosParameterName(direction, port.name, "reliability"),
      port.default_reliability,
      makeParameterDescriptor(
        "QoS reliability for port '" + port.name + "'.",
        "Supported values: best_effort, reliable."));
    declareParameterWithDescriptor(
      portQosParameterName(direction, port.name, "history"),
      port.default_history,
      makeParameterDescriptor(
        "QoS history policy for port '" + port.name + "'.",
        "Supported values: keep_last, keep_all."));
    declareParameterWithDescriptor(
      portQosParameterName(direction, port.name, "depth"),
      port.default_depth,
      makeIntegerRangeParameterDescriptor(
        "QoS history depth for port '" + port.name + "'.",
        1,
        100000));
    declareParameterWithDescriptor(
      portQosParameterName(direction, port.name, "durability"),
      port.default_durability,
      makeParameterDescriptor(
        "QoS durability for port '" + port.name + "'.",
        "Supported values: volatile, transient_local."));
  }

  void configureShmView()
  {
    std::unordered_map<std::string, std::string> remappings;
    for (const auto & key : shm_keys_) {
      remappings[key.name] = getParameter<std::string>(shmKeyParameterName(key.name));
    }
    shm_view_ = std::make_unique<component_shm::ShmView>();
    shm_view_->set_remappings(std::move(remappings));
  }

  const ShmKeyDescriptor & declaredShmKey(const std::string & key) const
  {
    const auto iter = std::find_if(
      shm_keys_.begin(),
      shm_keys_.end(),
      [&key](const auto & descriptor) {return descriptor.name == key;});
    if (iter == shm_keys_.end()) {
      throw std::out_of_range("Shared-memory key '" + key + "' is not declared");
    }
    return *iter;
  }

  template<typename T>
  const ShmKeyDescriptor & verifyShmKey(const std::string & key) const
  {
    (void)checkedShmView();
    using StoredType = std::remove_cv_t<T>;
    const auto & descriptor = declaredShmKey(key);
    if (descriptor.type_index != std::type_index(typeid(StoredType))) {
      throw std::invalid_argument("Shared-memory key '" + key + "' was declared with a different type");
    }
    return descriptor;
  }

  component_shm::ShmView & checkedShmView()
  {
    if (!shm_view_) {
      throw std::runtime_error("Shared-memory view is not configured");
    }
    return *shm_view_;
  }

  const component_shm::ShmView & checkedShmView() const
  {
    if (!shm_view_) {
      throw std::runtime_error("Shared-memory view is not configured");
    }
    return *shm_view_;
  }

  template<typename T>
  const ShmKeyDescriptor & verifyWritableShmKey(const std::string & key) const
  {
    const auto & descriptor = verifyShmKey<T>(key);
    if (descriptor.access != ShmAccess::ReadWrite) {
      throw std::runtime_error("Shared-memory key '" + key + "' is read-only");
    }
    return descriptor;
  }

  rclcpp::QoS portQos(const std::string & direction, const std::string & port_name)
  {
    const auto reliability = getParameter<std::string>(
      portQosParameterName(direction, port_name, "reliability"));
    const auto history = getParameter<std::string>(
      portQosParameterName(direction, port_name, "history"));
    const auto depth_value = getParameter<int>(
      portQosParameterName(direction, port_name, "depth"));
    const auto durability = getParameter<std::string>(
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

  template <typename T>
  void declareParameterWithDescriptor(
    const std::string & name,
    const T & default_value,
    const rcl_interfaces::msg::ParameterDescriptor & descriptor)
  {
    this->declare_parameter<T>(name, default_value, descriptor);
  }

  static rcl_interfaces::msg::ParameterDescriptor makeParameterDescriptor(
    const std::string & description,
    const std::string & additional_constraints = {})
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description = description;
    descriptor.additional_constraints = additional_constraints;
    return descriptor;
  }

  static rcl_interfaces::msg::ParameterDescriptor makeIntegerRangeParameterDescriptor(
    const std::string & description,
    std::int64_t from_value,
    std::int64_t to_value,
    std::uint64_t step = 1U,
    const std::string & additional_constraints = {})
  {
    auto descriptor = makeParameterDescriptor(description, additional_constraints);
    rcl_interfaces::msg::IntegerRange range;
    range.from_value = from_value;
    range.to_value = to_value;
    range.step = step;
    descriptor.integer_range.push_back(range);
    return descriptor;
  }

  static rcl_interfaces::msg::ParameterDescriptor makeFloatingPointRangeParameterDescriptor(
    const std::string & description,
    double from_value,
    double to_value,
    double step = 0.0,
    const std::string & additional_constraints = {})
  {
    auto descriptor = makeParameterDescriptor(description, additional_constraints);
    rcl_interfaces::msg::FloatingPointRange range;
    range.from_value = from_value;
    range.to_value = to_value;
    range.step = step;
    descriptor.floating_point_range.push_back(range);
    return descriptor;
  }

  bool active_{false};
  std::unique_ptr<component_shm::ShmView> shm_view_;
  std::unique_ptr<filter_component_synchronizer::FilterComponentSynchronizer> synchronizer_;
  std::unordered_map<std::string, std::unique_ptr<PublisherConcept>> publishers_;
};

}  // namespace filter_component_base::ros

#endif  // FILTER_COMPONENT_BASE__ROS__FILTER_COMPONENT_BASE_HPP_
