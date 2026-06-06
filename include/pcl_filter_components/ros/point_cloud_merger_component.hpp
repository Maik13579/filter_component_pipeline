// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_

#include <memory>
#include <mutex>
#include <string>

#include <lifecycle_msgs/msg/state.hpp>
#include <rclcpp/create_publisher.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "pcl_filter_components/ros/parameter_utils.hpp"
#include "pcl_filter_components/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT>
class PointCloudMergerComponent : public rclcpp_lifecycle::LifecycleNode
{
public:
  using StampedCloud = pcl::PointCloud<PointT>;
  using CloudAdapter = pcl_filter_components::ros::PclCloudAdapter<PointT>;
  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  explicit PointCloudMergerComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : rclcpp_lifecycle::LifecycleNode("point_cloud_merger", options)
  {
    declareParameterIfNotDeclared(
      *this,
      "input_topic_a",
      std::string{"/points/input_a"},
      makeParameterDescriptor("First input point cloud topic."));
    declareParameterIfNotDeclared(
      *this,
      "input_topic_b",
      std::string{"/points/input_b"},
      makeParameterDescriptor("Second input point cloud topic."));
    declareParameterIfNotDeclared(
      *this,
      "output_topic",
      std::string{"/points/output"},
      makeParameterDescriptor("Merged output point cloud topic."));
    declareParameterIfNotDeclared(
      *this,
      "queue_size",
      5,
      makeIntegerRangeParameterDescriptor("Depth used for subscriptions and publisher.", 1, 100000));
  }

protected:
  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override
  {
    (void)previous_state;
    const auto input_topic_a = getParameter<std::string>(*this, "input_topic_a");
    const auto input_topic_b = getParameter<std::string>(*this, "input_topic_b");
    const auto output_topic = getParameter<std::string>(*this, "output_topic");
    const auto queue_size = getParameter<int>(*this, "queue_size");
    const auto depth = queue_size > 0 ? static_cast<size_t>(queue_size) : 1U;
    const auto qos = rclcpp::SensorDataQoS().keep_last(depth);

    pub_ = rclcpp::create_publisher<CloudAdapter>(*this, output_topic, qos);
    sub_a_ = this->create_subscription<CloudAdapter>(
      input_topic_a,
      qos,
      [this](std::unique_ptr<StampedCloud> cloud) {cloudCallback(0U, std::move(cloud));});
    sub_b_ = this->create_subscription<CloudAdapter>(
      input_topic_b,
      qos,
      [this](std::unique_ptr<StampedCloud> cloud) {cloudCallback(1U, std::move(cloud));});
    active_ = false;
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
    sub_a_.reset();
    sub_b_.reset();
    pub_.reset();
    latest_a_.reset();
    latest_b_.reset();
    active_ = false;
    return CallbackReturn::SUCCESS;
  }

private:
  void cloudCallback(size_t index, std::unique_ptr<StampedCloud> cloud)
  {
    if (
      !active_ ||
      this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
    {
      return;
    }

    std::lock_guard<std::mutex> lock{mutex_};
    if (index == 0U) {
      latest_a_ = std::move(cloud);
    } else {
      latest_b_ = std::move(cloud);
    }
    if (!latest_a_ || !latest_b_) {
      return;
    }

    auto merged = std::make_unique<StampedCloud>();
    merged->header = latest_a_->header;
    merged->reserve(latest_a_->size() + latest_b_->size());
    merged->insert(merged->end(), latest_a_->begin(), latest_a_->end());
    merged->insert(merged->end(), latest_b_->begin(), latest_b_->end());
    merged->width = static_cast<uint32_t>(merged->size());
    merged->height = 1U;
    merged->is_dense = latest_a_->is_dense && latest_b_->is_dense;
    pub_->publish(std::move(merged));
  }

  typename rclcpp::Publisher<CloudAdapter>::SharedPtr pub_;
  typename rclcpp::Subscription<CloudAdapter>::SharedPtr sub_a_;
  typename rclcpp::Subscription<CloudAdapter>::SharedPtr sub_b_;
  std::unique_ptr<StampedCloud> latest_a_;
  std::unique_ptr<StampedCloud> latest_b_;
  std::mutex mutex_;
  bool active_{false};
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
