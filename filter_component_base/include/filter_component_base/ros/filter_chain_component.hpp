// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_BASE__ROS__FILTER_CHAIN_COMPONENT_HPP_
#define FILTER_COMPONENT_BASE__ROS__FILTER_CHAIN_COMPONENT_HPP_

#include <array>
#include <memory>
#include <stdexcept>
#include <string>

#include <filters/filter_chain.hpp>
#include <rclcpp/rclcpp.hpp>

#include "filter_component_base/ros/filter_component_base.hpp"
#include "filter_component_base/ros/parameter_utils.hpp"

namespace filter_component_base::ros
{

template <typename AdapterT, typename TraitsT>
class FilterChainComponent : public FilterComponentBase
{
public:
  using MessageT = typename rclcpp::TypeAdapter<AdapterT>::custom_type;

  explicit FilterChainComponent(const rclcpp::NodeOptions & options)
  : FilterComponentBase(
      TraitsT::nodeName(),
      options,
      std::array{FilterComponentBase::template inputPort<AdapterT>(
          TraitsT::inputPort(),
          "Input stream processed by the ROS filters chain.")},
      std::array{FilterComponentBase::template outputPort<AdapterT>(
          TraitsT::outputPort(),
          "Output stream produced by the ROS filters chain.")})
  {
    declareParameterIfNotDeclared(
      *this,
      "in_place",
      false,
      makeParameterDescriptor(
        "Process the filter chain in-place and publish the input message pointer."));
  }

protected:
  void configure() override
  {
    in_place_ = getParameter<bool>(*this, "in_place");
    filter_chain_ = std::make_unique<filters::FilterChain<MessageT>>(TraitsT::dataType());
    if (!filter_chain_->configure(
        "filters",
        get_node_logging_interface(),
        get_node_parameters_interface()))
    {
      filter_chain_.reset();
      throw std::runtime_error(
        "Failed to configure filters::FilterChain for data type '" +
        std::string{TraitsT::dataType()} + "'");
    }
  }

  void cleanupInterfaces() override
  {
    FilterComponentBase::cleanupInterfaces();
    filter_chain_.reset();
  }

  void process() override
  {
    auto input = takeInput<AdapterT>(TraitsT::inputPort());
    if (!input) {
      return;
    }

    if (in_place_) {
      if (!filter_chain_ || !filter_chain_->update(*input, *input)) {
        RCLCPP_WARN(
          get_logger(),
          "filters::FilterChain in-place update failed for data type '%s'; dropping message",
          TraitsT::dataType());
        return;
      }
      publish<AdapterT>(TraitsT::outputPort(), std::move(input));
      return;
    }

    auto output = std::make_unique<MessageT>();
    if (!filter_chain_ || !filter_chain_->update(*input, *output)) {
      RCLCPP_WARN(
        get_logger(),
        "filters::FilterChain update failed for data type '%s'; dropping message",
        TraitsT::dataType());
      return;
    }
    publish<AdapterT>(TraitsT::outputPort(), std::move(output));
  }

private:
  std::unique_ptr<filters::FilterChain<MessageT>> filter_chain_;
  bool in_place_{false};
};

}  // namespace filter_component_base::ros

#endif  // FILTER_COMPONENT_BASE__ROS__FILTER_CHAIN_COMPONENT_HPP_
