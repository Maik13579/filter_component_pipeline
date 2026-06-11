// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "component_shm/shared_memory.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"

namespace
{

using filter_component_base::ros::FilterComponentBase;
using filter_component_base::ros::PortDescriptor;
using filter_component_base::ros::ShmAccess;

rclcpp::NodeOptions optionsWithOverrides(std::vector<rclcpp::Parameter> overrides = {})
{
  auto options = rclcpp::NodeOptions{};
  options.parameter_overrides(std::move(overrides));
  return options;
}

class ShmFixtureComponent : public FilterComponentBase
{
public:
  explicit ShmFixtureComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions{})
  : FilterComponentBase(
      "shm_fixture_component",
      options,
      std::array<PortDescriptor, 0U>{},
      std::array<PortDescriptor, 0U>{},
      std::array{
        shmKey<int>("read_value", "Read-only integer value.", ShmAccess::ReadOnly, "int"),
        shmKey<std::string>("write_value", "Writable string value.", ShmAccess::ReadWrite, "std::string")})
  {
  }

  using FilterComponentBase::shmGet;
  using FilterComponentBase::shmGetMutable;
  using FilterComponentBase::shmSet;
  using FilterComponentBase::shmSetShared;
  using FilterComponentBase::shmTryGet;
  using FilterComponentBase::shmTryGetMutable;
  using FilterComponentBase::shmView;

  void configure() override {}
  void process() override {}
};

class ShmComponentTest : public ::testing::Test
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

  void SetUp() override
  {
    component_shm::SharedMemory::instance()->clear();
  }
};

void configureLifecycle(
  const std::shared_ptr<ShmFixtureComponent> & component,
  FilterComponentBase::CallbackReturn & callback_return)
{
  static_cast<rclcpp_lifecycle::LifecycleNode *>(component.get())->configure(callback_return);
}

void cleanupLifecycle(
  const std::shared_ptr<ShmFixtureComponent> & component,
  FilterComponentBase::CallbackReturn & callback_return)
{
  static_cast<rclcpp_lifecycle::LifecycleNode *>(component.get())->cleanup(callback_return);
}

TEST_F(ShmComponentTest, DeclaresShmKeyParametersWithDefaultsAndDescriptions)
{
  auto component = std::make_shared<ShmFixtureComponent>();

  EXPECT_EQ(component->get_parameter("shm_key.read_value").as_string(), "read_value");
  EXPECT_EQ(component->get_parameter("shm_key.write_value").as_string(), "write_value");

  const auto parameters = component->describe_parameters({
    "shm_key.read_value",
    "shm_key.write_value",
  });
  ASSERT_EQ(parameters.size(), 2U);
  EXPECT_EQ(parameters[0].description, "Read-only integer value.");
  EXPECT_EQ(parameters[1].description, "Writable string value.");
}

TEST_F(ShmComponentTest, ConfigurePopulatesShmViewRemappings)
{
  auto component = std::make_shared<ShmFixtureComponent>(
    optionsWithOverrides({
      rclcpp::Parameter{"shm_key.read_value", "shared/read"},
      rclcpp::Parameter{"shm_key.write_value", "shared/write"},
    }));

  FilterComponentBase::CallbackReturn callback_return;
  configureLifecycle(component, callback_return);

  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);
  const auto remappings = component->shmView().get_remappings();
  EXPECT_EQ(remappings.at("read_value"), "shared/read");
  EXPECT_EQ(remappings.at("write_value"), "shared/write");
}

TEST_F(ShmComponentTest, ConfigureRecreatesShmViewForUpdatedRemappings)
{
  auto component = std::make_shared<ShmFixtureComponent>(
    optionsWithOverrides({
      rclcpp::Parameter{"shm_key.read_value", "shared/read"},
      rclcpp::Parameter{"shm_key.write_value", "shared/write"},
    }));

  FilterComponentBase::CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);
  const_cast<component_shm::ShmView &>(component->shmView()).addRemapping("temporary", "leaked");

  cleanupLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);
  component->set_parameter(rclcpp::Parameter{"shm_key.read_value", "shared/read_after_reconfigure"});
  configureLifecycle(component, callback_return);

  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);
  const auto remappings = component->shmView().get_remappings();
  EXPECT_EQ(remappings.at("read_value"), "shared/read_after_reconfigure");
  EXPECT_EQ(remappings.at("write_value"), "shared/write");
  EXPECT_EQ(remappings.count("temporary"), 0U);
}

TEST_F(ShmComponentTest, ReadOnlyHelpersRejectMutableAndWriteOperations)
{
  auto component = std::make_shared<ShmFixtureComponent>();
  FilterComponentBase::CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);

  component_shm::SharedMemory::instance()->set<int>("read_value", 42);

  ASSERT_NE(component->shmGet<int>("read_value"), nullptr);
  EXPECT_EQ(*component->shmGet<int>("read_value"), 42);
  EXPECT_THROW((void)component->shmGetMutable<int>("read_value"), std::runtime_error);
  EXPECT_THROW(component->shmSet<int>("read_value", 43), std::runtime_error);
  EXPECT_THROW(component->shmSetShared<int>("read_value", std::make_shared<int>(44)), std::runtime_error);
}

TEST_F(ShmComponentTest, ReadWriteHelpersAllowGetSetAndSetShared)
{
  auto component = std::make_shared<ShmFixtureComponent>();
  FilterComponentBase::CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);

  component->shmSet<std::string>("write_value", "first");
  ASSERT_NE(component->shmGet<std::string>("write_value"), nullptr);
  EXPECT_EQ(*component->shmGet<std::string>("write_value"), "first");

  auto mutable_value = component->shmGetMutable<std::string>("write_value");
  ASSERT_NE(mutable_value, nullptr);
  *mutable_value = "mutated";
  EXPECT_EQ(*component->shmGet<std::string>("write_value"), "mutated");

  component->shmSetShared<std::string>("write_value", std::make_shared<std::string>("shared"));
  ASSERT_NE(component->shmTryGet<std::string>("write_value"), nullptr);
  EXPECT_EQ(*component->shmTryGet<std::string>("write_value"), "shared");
  ASSERT_NE(component->shmTryGetMutable<std::string>("write_value"), nullptr);
}

TEST_F(ShmComponentTest, TypeMismatchAndUndeclaredKeysThrow)
{
  auto component = std::make_shared<ShmFixtureComponent>();
  FilterComponentBase::CallbackReturn callback_return;
  configureLifecycle(component, callback_return);
  ASSERT_EQ(callback_return, FilterComponentBase::CallbackReturn::SUCCESS);

  EXPECT_THROW((void)component->shmGet<double>("read_value"), std::invalid_argument);
  EXPECT_THROW(component->shmSet<int>("write_value", 5), std::invalid_argument);
  EXPECT_THROW((void)component->shmTryGet<int>("missing"), std::out_of_range);
  EXPECT_THROW(component->shmSet<std::string>("missing", "value"), std::out_of_range);
}

}  // namespace
