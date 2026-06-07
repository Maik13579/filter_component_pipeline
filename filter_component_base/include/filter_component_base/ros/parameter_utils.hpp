// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_BASE__ROS__PARAMETER_UTILS_HPP_
#define FILTER_COMPONENT_BASE__ROS__PARAMETER_UTILS_HPP_

#include <cstdint>
#include <string>

#include <rcl_interfaces/msg/floating_point_range.hpp>
#include <rcl_interfaces/msg/integer_range.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

namespace filter_component_base::ros
{

template <typename T>
void declareParameterIfNotDeclared(
  rclcpp_lifecycle::LifecycleNode & node,
  const std::string & name,
  const T & default_value)
{
  if (!node.has_parameter(name)) {
    node.declare_parameter<T>(name, default_value);
  }
}

template <typename T>
void declareParameterIfNotDeclared(
  rclcpp_lifecycle::LifecycleNode & node,
  const std::string & name,
  const T & default_value,
  const rcl_interfaces::msg::ParameterDescriptor & descriptor)
{
  if (!node.has_parameter(name)) {
    node.declare_parameter<T>(name, default_value, descriptor);
  }
}

inline rcl_interfaces::msg::ParameterDescriptor makeParameterDescriptor(
  const std::string & description,
  const std::string & additional_constraints = {})
{
  rcl_interfaces::msg::ParameterDescriptor descriptor;
  descriptor.description = description;
  descriptor.additional_constraints = additional_constraints;
  return descriptor;
}

inline rcl_interfaces::msg::ParameterDescriptor makeIntegerRangeParameterDescriptor(
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

inline rcl_interfaces::msg::ParameterDescriptor makeFloatingPointRangeParameterDescriptor(
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

template <typename T>
T getParameter(
  const rclcpp_lifecycle::LifecycleNode & node,
  const std::string & name)
{
  return node.get_parameter(name).get_value<T>();
}

}  // namespace filter_component_base::ros

#endif  // FILTER_COMPONENT_BASE__ROS__PARAMETER_UTILS_HPP_
