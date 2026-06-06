// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__PCL_FILTER_COMPONENT_BASE_HPP_
#define PCL_FILTER_COMPONENTS__ROS__PCL_FILTER_COMPONENT_BASE_HPP_

#include <memory>
#include <string>

#include <rclcpp/create_publisher.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "pcl_filter_components/ros/parameter_utils.hpp"
#include "pcl_filter_components/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT, typename FilterT>
class PclFilterComponentBase : public rclcpp_lifecycle::LifecycleNode
{
public:
  using StampedCloud = pcl::PointCloud<PointT>;
  using Cloud = StampedCloud;
  using CloudAdapter = pcl_filter_components::ros::PclCloudAdapter<PointT>;
  using Publisher = rclcpp::Publisher<CloudAdapter>;
  using Subscription = rclcpp::Subscription<CloudAdapter>;
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  explicit PclFilterComponentBase(
    const std::string & node_name,
    const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : rclcpp_lifecycle::LifecycleNode(node_name, options)
  {
    declareParameterIfNotDeclared(
      *this,
      "input_topic",
      std::string{"/points/input"},
      makeParameterDescriptor("Input point cloud topic subscribed by the filter."));
    declareParameterIfNotDeclared(
      *this,
      "output_topic",
      std::string{"/points/output"},
      makeParameterDescriptor("Output topic published by the filter."));
    declareParameterIfNotDeclared(
      *this,
      "queue_size",
      5,
      makeIntegerRangeParameterDescriptor(
        "Depth used for input subscriptions and output publishers.",
        1,
        100000));
  }

protected:
  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;

    input_topic_ = getParameter<std::string>(*this, "input_topic");
    output_topic_ = getParameter<std::string>(*this, "output_topic");
    queue_size_ = getParameter<int>(*this, "queue_size");

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
    pub_ = createAdaptedPublisher<CloudAdapter>(output_topic_, qos);
    sub_ = this->create_subscription<CloudAdapter>(
      input_topic_,
      qos,
      std::bind(&PclFilterComponentBase::cloudCallback, this, std::placeholders::_1));
  }

  virtual void cleanupInterfaces()
  {
    sub_.reset();
    pub_.reset();
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

  void publishCloud(std::unique_ptr<StampedCloud> output)
  {
    pub_->publish(std::move(output));
  }

  void cloudCallback(std::unique_ptr<StampedCloud> input)
  {
    if (
      !active_ ||
      this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
    {
      return;
    }
    processCloud(std::move(input));
  }

  FilterT filter_;
  typename Publisher::SharedPtr pub_;
  typename Subscription::SharedPtr sub_;
  std::string input_topic_{"/points/input"};
  std::string output_topic_{"/points/output"};
  int queue_size_{5};

private:
  bool active_{false};
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__PCL_FILTER_COMPONENT_BASE_HPP_
