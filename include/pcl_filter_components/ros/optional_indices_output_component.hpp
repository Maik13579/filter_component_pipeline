// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_

#include <functional>
#include <memory>
#include <string>

#include <pcl/PointIndices.h>
#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/ros/parameter_utils.hpp"
#include "pcl_filter_components/ros/pcl_filter_component_base.hpp"
#include "pcl_filter_components/ros/stamped_pcl_indices_type_adapter.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT, typename FilterT>
class OptionalIndicesOutputComponent
  : public PclFilterComponentBase<PointT, FilterT>
{
public:
  OptionalIndicesOutputComponent(
    const std::string & node_name,
    const rclcpp::NodeOptions & options)
  : PclFilterComponentBase<PointT, FilterT>(node_name, options)
  {
    declareParameterIfNotDeclared(
      *this,
      "filter.output_indices",
      false,
      makeParameterDescriptor(
        "Publish filtered point indices instead of a filtered point cloud when enabled."));
  }

protected:
  using Base = PclFilterComponentBase<PointT, FilterT>;
  using StampedCloud = typename Base::StampedCloud;
  using CloudAdapter = typename Base::CloudAdapter;

  void configureInterfaces(const rclcpp::QoS & qos) override
  {
    output_indices_ = getParameter<bool>(*this, "filter.output_indices");
    if (output_indices_) {
      indices_pub_ = this->template createAdaptedPublisher<PclIndicesAdapter>(
        this->output_topic_,
        qos);
      this->sub_ = this->template create_subscription<CloudAdapter>(
        this->input_topic_,
        qos,
        std::bind(
          &OptionalIndicesOutputComponent::cloudCallback,
          this,
          std::placeholders::_1));
      return;
    }

    Base::configureInterfaces(qos);
  }

  void cleanupInterfaces() override
  {
    indices_pub_.reset();
    Base::cleanupInterfaces();
  }

  void processCloud(std::unique_ptr<StampedCloud> input) override
  {
    if (!output_indices_) {
      Base::processCloud(std::move(input));
      return;
    }

    auto output = std::make_unique<pcl::PointIndices>();
    output->header = input->header;
    this->filter_.filterIndices(*input, output->indices);
    indices_pub_->publish(std::move(output));
  }

private:
  rclcpp::Publisher<PclIndicesAdapter>::SharedPtr indices_pub_;
  bool output_indices_{false};
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_
