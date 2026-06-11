// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_BASE__ROS__COMPONENT_DESCRIPTORS_HPP_
#define FILTER_COMPONENT_BASE__ROS__COMPONENT_DESCRIPTORS_HPP_

#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <utility>
#include <utility>

#include <rclcpp/rclcpp.hpp>

#include "filter_component_synchronizer/filter_component_synchronizer.hpp"

namespace filter_component_base::ros
{

class FilterComponentBase;

/// @brief Access mode for a shared-memory key declared by a filter component.
enum class ShmAccess
{
  ReadOnly,
  ReadWrite,
};

/// @brief Describes one shared-memory key used by a filter component.
struct ShmKeyDescriptor
{
  std::string name;
  std::string description;
  std::string type_name;
  std::type_index type_index;
  ShmAccess access{ShmAccess::ReadOnly};
};

/// @brief Type-erased publisher storage for an output port.
struct PublisherConcept
{
  explicit PublisherConcept(std::type_index adapter_type_in)
  : adapter_type(adapter_type_in)
  {
  }

  virtual ~PublisherConcept() = default;
  std::type_index adapter_type;
};

/// @brief Typed publisher storage for an output port adapter.
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

/// @brief Describes an input or output topic port of a filter component.
struct PortDescriptor
{
  std::string name;
  std::string description;
  std::string type_name;
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

}  // namespace filter_component_base::ros

#endif  // FILTER_COMPONENT_BASE__ROS__COMPONENT_DESCRIPTORS_HPP_
