// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/type_adapter.hpp>
#include <std_msgs/msg/u_int64.hpp>

#include "filter_component_synchronizer/filter_component_synchronizer.hpp"

namespace
{

struct TestHeader
{
  uint64_t stamp{0U};
};

struct TestStamped
{
  TestHeader header;
  uint64_t value{0U};
};

struct OtherStamped
{
  TestHeader header;
  uint64_t value{0U};
};

struct TestUnheadered
{
  uint64_t timestamp{0U};
  uint64_t value{0U};
};

}  // namespace

namespace rclcpp
{

template <>
struct TypeAdapter<TestStamped, std_msgs::msg::UInt64>
{
  using is_specialized = std::true_type;
  using custom_type = TestStamped;
  using ros_message_type = std_msgs::msg::UInt64;

  static void convert_to_ros_message(const custom_type & source, ros_message_type & destination)
  {
    destination.data = source.header.stamp;
  }

  static void convert_to_custom(const ros_message_type & source, custom_type & destination)
  {
    destination.header.stamp = source.data;
    destination.value = source.data;
  }
};

template <>
struct TypeAdapter<OtherStamped, std_msgs::msg::UInt64>
{
  using is_specialized = std::true_type;
  using custom_type = OtherStamped;
  using ros_message_type = std_msgs::msg::UInt64;

  static void convert_to_ros_message(const custom_type & source, ros_message_type & destination)
  {
    destination.data = source.header.stamp;
  }

  static void convert_to_custom(const ros_message_type & source, custom_type & destination)
  {
    destination.header.stamp = source.data;
    destination.value = source.data;
  }
};

template <>
struct TypeAdapter<TestUnheadered, std_msgs::msg::UInt64>
{
  using is_specialized = std::true_type;
  using custom_type = TestUnheadered;
  using ros_message_type = std_msgs::msg::UInt64;

  static void convert_to_ros_message(const custom_type & source, ros_message_type & destination)
  {
    destination.data = source.timestamp;
  }

  static void convert_to_custom(const ros_message_type & source, custom_type & destination)
  {
    destination.timestamp = source.data;
    destination.value = source.data;
  }
};

}  // namespace rclcpp

namespace
{

using TestAdapter = rclcpp::adapt_type<TestStamped>::as<std_msgs::msg::UInt64>;
using OtherAdapter = rclcpp::adapt_type<OtherStamped>::as<std_msgs::msg::UInt64>;
using UnheaderedAdapter = rclcpp::adapt_type<TestUnheadered>::as<std_msgs::msg::UInt64>;

class SynchronizerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    context = std::make_shared<rclcpp::Context>();
    char const * argv[] = {"test_filter_component_synchronizer"};
    context->init(1, argv);
    node = std::make_shared<rclcpp::Node>(
      "test_filter_component_synchronizer",
      rclcpp::NodeOptions{}.context(context).use_intra_process_comms(true));
    rclcpp::ExecutorOptions executor_options;
    executor_options.context = context;
    executor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>(executor_options);
    executor->add_node(node);
  }

  void TearDown() override
  {
    executor->remove_node(node);
    node.reset();
    executor.reset();
    context->shutdown("test complete");
    context.reset();
  }

  void spinFor(std::chrono::milliseconds duration)
  {
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < deadline) {
      executor->spin_some(std::chrono::milliseconds{5});
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
  }

  void publishStamp(
    rclcpp::Publisher<TestAdapter>::SharedPtr publisher,
    uint64_t stamp,
    std::chrono::milliseconds spin_duration = std::chrono::milliseconds{50})
  {
    auto message = std::make_unique<TestStamped>();
    message->header.stamp = stamp;
    message->value = stamp;
    publisher->publish(std::move(message));
    spinFor(spin_duration);
  }

  void publishUnheaderedStamp(
    rclcpp::Publisher<UnheaderedAdapter>::SharedPtr publisher,
    uint64_t stamp,
    std::chrono::milliseconds spin_duration = std::chrono::milliseconds{50})
  {
    auto message = std::make_unique<TestUnheadered>();
    message->timestamp = stamp;
    message->value = stamp;
    publisher->publish(std::move(message));
    spinFor(spin_duration);
  }

  std::shared_ptr<rclcpp::Context> context;
  std::shared_ptr<rclcpp::Node> node;
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor;
};

TEST_F(SynchronizerTest, OneInputPassThroughTriggers)
{
  size_t ready_count = 0U;
  filter_component_synchronizer::FilterComponentSynchronizer sync{{}, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{1};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "cloud", "cloud_in", qos);
  auto publisher = node->create_publisher<TestAdapter>("cloud_in", qos);

  publishStamp(publisher, 10U);

  EXPECT_EQ(ready_count, 1U);
  auto input = sync.takeInput<TestAdapter>("cloud");
  ASSERT_NE(input, nullptr);
  EXPECT_EQ(input->header.stamp, 10U);
}

TEST_F(SynchronizerTest, TwoInputsMatchWhenReceiptSpanIsWithinMaxInterval)
{
  size_t ready_count = 0U;
  auto options = filter_component_synchronizer::SynchronizerOptions{};
  options.max_interval = 1.0;
  filter_component_synchronizer::FilterComponentSynchronizer sync{options, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{5};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "left", "left_in", qos);
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "right", "right_in", qos);
  auto left = node->create_publisher<TestAdapter>("left_in", qos);
  auto right = node->create_publisher<TestAdapter>("right_in", qos);

  publishStamp(left, 100U);
  EXPECT_EQ(ready_count, 0U);
  publishStamp(right, 200U);

  EXPECT_EQ(ready_count, 1U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("left")->header.stamp, 100U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("right")->header.stamp, 200U);
}

TEST_F(SynchronizerTest, InputsDoNotMatchWhenReceiptSpanExceedsMaxInterval)
{
  size_t ready_count = 0U;
  auto options = filter_component_synchronizer::SynchronizerOptions{};
  options.max_interval = 0.001;
  filter_component_synchronizer::FilterComponentSynchronizer sync{options, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{5};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "left", "too_old_left", qos);
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "right", "too_old_right", qos);
  auto left = node->create_publisher<TestAdapter>("too_old_left", qos);
  auto right = node->create_publisher<TestAdapter>("too_old_right", qos);

  publishStamp(left, 1U);
  publishStamp(right, 2U);

  EXPECT_EQ(ready_count, 0U);
  EXPECT_EQ(sync.peekInput<TestAdapter>("left"), nullptr);
  EXPECT_EQ(sync.peekInput<TestAdapter>("right"), nullptr);
}

TEST_F(SynchronizerTest, QueueOverflowDropsOldUnmatchedInputs)
{
  size_t ready_count = 0U;
  auto options = filter_component_synchronizer::SynchronizerOptions{};
  options.max_interval = 1.0;
  options.queue_size = 1U;
  filter_component_synchronizer::FilterComponentSynchronizer sync{options, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{5};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "left", "overflow_left", qos);
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "right", "overflow_right", qos);
  auto left = node->create_publisher<TestAdapter>("overflow_left", qos);
  auto right = node->create_publisher<TestAdapter>("overflow_right", qos);

  publishStamp(left, 1U);
  publishStamp(left, 2U);
  publishStamp(right, 9U);

  EXPECT_EQ(ready_count, 1U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("left")->header.stamp, 2U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("right")->header.stamp, 9U);
}

TEST_F(SynchronizerTest, StaleMessagesOlderThanMaxIntervalAreDropped)
{
  size_t ready_count = 0U;
  auto options = filter_component_synchronizer::SynchronizerOptions{};
  options.max_interval = 0.01;
  filter_component_synchronizer::FilterComponentSynchronizer sync{options, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{5};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "left", "stale_left", qos);
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "right", "stale_right", qos);
  auto left = node->create_publisher<TestAdapter>("stale_left", qos);
  auto right = node->create_publisher<TestAdapter>("stale_right", qos);

  publishStamp(left, 1U);
  publishStamp(right, 2U);
  EXPECT_EQ(ready_count, 0U);

  publishStamp(left, 3U, std::chrono::milliseconds{2});
  publishStamp(right, 4U, std::chrono::milliseconds{2});

  EXPECT_EQ(ready_count, 1U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("left")->header.stamp, 3U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("right")->header.stamp, 4U);
}

TEST_F(SynchronizerTest, TypedAccessorRejectsWrongAdapterType)
{
  filter_component_synchronizer::FilterComponentSynchronizer sync;
  const auto qos = rclcpp::QoS{1};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "cloud", "typed_cloud", qos);

  EXPECT_THROW(sync.takeInput<OtherAdapter>("cloud"), std::invalid_argument);
}

TEST_F(SynchronizerTest, HeaderlessAdaptedMessageDoesNotNeedInputStampTrait)
{
  size_t ready_count = 0U;
  filter_component_synchronizer::FilterComponentSynchronizer sync{{}, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{1};
  sync.addInput<rclcpp::Node, UnheaderedAdapter>(*node, "image", "image_in", qos);
  auto publisher = node->create_publisher<UnheaderedAdapter>("image_in", qos);

  publishUnheaderedStamp(publisher, 99U);

  EXPECT_EQ(ready_count, 1U);
  auto input = sync.takeInput<UnheaderedAdapter>("image");
  ASSERT_NE(input, nullptr);
  EXPECT_EQ(input->timestamp, 99U);
}

TEST_F(SynchronizerTest, LatestModeFiresWithMostRecentDataOnAnyInput)
{
  size_t ready_count = 0U;
  auto options = filter_component_synchronizer::SynchronizerOptions{};
  options.mode = filter_component_synchronizer::SyncMode::Latest;
  filter_component_synchronizer::FilterComponentSynchronizer sync{options, [&ready_count]() {++ready_count;}};
  const auto qos = rclcpp::QoS{5};
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "left", "latest_left", qos);
  sync.addInput<rclcpp::Node, TestAdapter>(*node, "right", "latest_right", qos);
  auto left = node->create_publisher<TestAdapter>("latest_left", qos);
  auto right = node->create_publisher<TestAdapter>("latest_right", qos);

  publishStamp(left, 1U);
  EXPECT_EQ(ready_count, 0U);
  publishStamp(right, 2U);
  EXPECT_EQ(ready_count, 1U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("left")->header.stamp, 1U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("right")->header.stamp, 2U);

  publishStamp(left, 3U);
  EXPECT_EQ(ready_count, 2U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("left")->header.stamp, 3U);
  EXPECT_EQ(sync.takeInput<TestAdapter>("right")->header.stamp, 2U);
}

}  // namespace
