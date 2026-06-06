// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_BASE__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_
#define PCL_FILTER_BASE__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_

#include <functional>
#include <memory>
#include <string>

#include <pcl/PointIndices.h>
#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_base/ros/parameter_utils.hpp"
#include "pcl_filter_base/ros/pcl_filter_component_base.hpp"
#include "pcl_filter_type_adapters/ros/stamped_pcl_indices_type_adapter.hpp"

namespace pcl_filter_base::ros
{

using PclIndicesAdapter = pcl_filter_type_adapters::ros::PclIndicesAdapter;

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

  void processCloud(std::unique_ptr<StampedCloud> input) override
  {
    if (!output_indices_) {
      Base::processCloud(std::move(input));
      return;
    }

    auto output = std::make_unique<pcl::PointIndices>();
    output->header = input->header;
    this->filter_.filterIndices(*input, output->indices);
    this->template publish<PclIndicesAdapter>("indices", std::move(output));
  }

  void configureInterfaces(const rclcpp::QoS & qos) override
  {
    Base::configureInterfaces(qos);
    output_indices_ = getParameter<bool>(*this, "filter.output_indices");
    if (output_indices_) {
      this->publishers_["indices"] =
        std::make_unique<typename Base::template PublisherHolder<PclIndicesAdapter>>(
          this->template createAdaptedPublisher<PclIndicesAdapter>(
            this->output_topics_.at("indices"),
            qos));
    }
  }

private:
  bool output_indices_{false};
};

}  // namespace pcl_filter_base::ros

#endif  // PCL_FILTER_BASE__ROS__OPTIONAL_INDICES_OUTPUT_COMPONENT_HPP_
