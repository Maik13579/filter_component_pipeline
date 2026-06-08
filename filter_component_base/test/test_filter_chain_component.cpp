// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/type_adapter.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <std_msgs/msg/int32.hpp>

#include "filter_component_base/ros/filter_chain_component.hpp"

namespace rclcpp
{

template <>
struct TypeAdapter<int, std_msgs::msg::Int32>
{
  using is_specialized = std::true_type;
  using custom_type = int;
  using ros_message_type = std_msgs::msg::Int32;

  static void convert_to_ros_message(const custom_type & source, ros_message_type & destination)
  {
    destination.data = source;
  }

  static void convert_to_custom(const ros_message_type & source, custom_type & destination)
  {
    destination = source.data;
  }
};

}  // namespace rclcpp

namespace
{

using namespace std::chrono_literals;

using IntAdapter = rclcpp::adapt_type<int>::as<std_msgs::msg::Int32>;

struct IntChainTraits
{
  static const char * nodeName() {return "test_int_filter_chain";}
  static const char * dataType() {return "int";}
  static const char * inputPort() {return "value";}
  static const char * outputPort() {return "value";}
  static const char * defaultParamPrefix() {return "filter_chain";}
};

using IntFilterChainComponent =
  filter_component_base::ros::FilterChainComponent<IntAdapter, IntChainTraits>;
using CallbackReturn = IntFilterChainComponent::CallbackReturn;

rclcpp::NodeOptions optionsWithOverrides(std::vector<rclcpp::Parameter> overrides)
{
  auto options = rclcpp::NodeOptions{};
  options.parameter_overrides(std::move(overrides));
  return options;
}

class RosContextTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    if (!rclcpp::ok()) {
      rclcpp::init(0, nullptr);
    }
  }

  static void TearDownTestSuite()
  {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }
};

void spinUntil(
  rclcpp::executors::SingleThreadedExecutor & executor,
  const std::shared_ptr<bool> & done)
{
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (!*done && std::chrono::steady_clock::now() < deadline) {
    executor.spin_some(20ms);
    std::this_thread::sleep_for(10ms);
  }
}

void configureLifecycle(
  const std::shared_ptr<IntFilterChainComponent> & component,
  CallbackReturn & callback_return)
{
  static_cast<rclcpp_lifecycle::LifecycleNode *>(component.get())->configure(callback_return);
}

TEST_F(RosContextTest, EmptyChainCopiesInputToOutput)
{
  auto component = std::make_shared<IntFilterChainComponent>(
    optionsWithOverrides({
      rclcpp::Parameter{"inputs.value.topic", "/empty_chain/input"},
      rclcpp::Parameter{"outputs.value.topic", "/empty_chain/output"},
      rclcpp::Parameter{"inputs.value.qos.reliability", "reliable"},
      rclcpp::Parameter{"outputs.value.qos.reliability", "reliable"},
    }));
  auto io_node = std::make_shared<rclcpp::Node>("empty_chain_io");
  auto received = std::make_shared<int>(0);
  auto has_message = std::make_shared<bool>(false);

  auto publisher = io_node->create_publisher<IntAdapter>("/empty_chain/input", rclcpp::QoS{10}.reliable());
  auto subscription = io_node->create_subscription<IntAdapter>(
    "/empty_chain/output",
    rclcpp::QoS{10}.reliable(),
    [received, has_message](std::unique_ptr<int> message) {
      *received = *message;
      *has_message = true;
    });

  CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, CallbackReturn::SUCCESS);
  component->activate(callback_return);
  ASSERT_EQ(callback_return, CallbackReturn::SUCCESS);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(component->get_node_base_interface());
  executor.add_node(io_node);

  publisher->publish(std::make_unique<int>(41));
  spinUntil(executor, has_message);

  ASSERT_TRUE(*has_message);
  EXPECT_EQ(*received, 41);
}

TEST_F(RosContextTest, ConfiguredChainLoadsPluginAndPublishesResult)
{
  auto component = std::make_shared<IntFilterChainComponent>(
    optionsWithOverrides({
      rclcpp::Parameter{"inputs.value.topic", "/increment_chain/input"},
      rclcpp::Parameter{"outputs.value.topic", "/increment_chain/output"},
      rclcpp::Parameter{"inputs.value.qos.reliability", "reliable"},
      rclcpp::Parameter{"outputs.value.qos.reliability", "reliable"},
      rclcpp::Parameter{"filter_chain.filter1.name", "increment"},
      rclcpp::Parameter{"filter_chain.filter1.type", "filters/IncrementFilterInt"},
    }));
  auto io_node = std::make_shared<rclcpp::Node>("increment_chain_io");
  auto received = std::make_shared<int>(0);
  auto has_message = std::make_shared<bool>(false);

  auto publisher = io_node->create_publisher<IntAdapter>("/increment_chain/input", rclcpp::QoS{10}.reliable());
  auto subscription = io_node->create_subscription<IntAdapter>(
    "/increment_chain/output",
    rclcpp::QoS{10}.reliable(),
    [received, has_message](std::unique_ptr<int> message) {
      *received = *message;
      *has_message = true;
    });

  CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, CallbackReturn::SUCCESS);
  component->activate(callback_return);
  ASSERT_EQ(callback_return, CallbackReturn::SUCCESS);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(component->get_node_base_interface());
  executor.add_node(io_node);

  publisher->publish(std::make_unique<int>(41));
  spinUntil(executor, has_message);

  ASSERT_TRUE(*has_message);
  EXPECT_EQ(*received, 42);
}

TEST_F(RosContextTest, FailedChainConfigurationReturnsLifecycleFailure)
{
  auto component = std::make_shared<IntFilterChainComponent>(
    optionsWithOverrides({
      rclcpp::Parameter{"filter_chain.filter1.name", "missing"},
      rclcpp::Parameter{"filter_chain.filter1.type", "filters/MissingFilterInt"},
    }));

  CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  EXPECT_EQ(callback_return, CallbackReturn::FAILURE);
}

}  // namespace
