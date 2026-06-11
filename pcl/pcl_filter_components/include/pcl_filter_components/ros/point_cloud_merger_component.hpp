// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

using filter_component_base::ros::PortDescriptor;

namespace detail
{

template <typename PointT>
struct NoopFilter
{
  void filter(
    const pcl::PointCloud<PointT> & input,
    pcl::PointCloud<PointT> & output) const
  {
    output = input;
  }
};

}  // namespace detail

template <typename PointT>
class PointCloudMergerComponent
  : public filter_component_base::ros::FilterComponentBase
{
public:
  using Base = filter_component_base::ros::FilterComponentBase;
  using StampedCloud = pcl::PointCloud<PointT>;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;

  explicit PointCloudMergerComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "point_cloud_merger",
      options,
      inputPorts(),
      outputPorts())
  {
  }

protected:
  static std::array<PortDescriptor, 2> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>(
        "input_1",
        "First input point cloud topic."),
      Base::template inputPort<CloudAdapter>(
        "input_2",
        "Second input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 3> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "Merged output point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_1",
        "Original first input point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_input_2",
        "Original second input point cloud topic."),
    }};
  }

  void configure() override
  {
  }

  void process() override
  {
    auto first = this->template takeInput<CloudAdapter>("input_1");
    auto second = this->template takeInput<CloudAdapter>("input_2");
    if (!first || !second) {
      return;
    }

    auto merged = std::make_unique<StampedCloud>();
    merged->header = first->header;
    merged->reserve(first->size() + second->size());
    merged->insert(merged->end(), first->begin(), first->end());
    merged->insert(merged->end(), second->begin(), second->end());
    merged->width = static_cast<uint32_t>(merged->size());
    merged->height = 1U;
    merged->is_dense = first->is_dense && second->is_dense;
    this->template publish<CloudAdapter>("cloud", std::move(merged));
    this->template publish<CloudAdapter>("orig_input_1", std::move(first));
    this->template publish<CloudAdapter>("orig_input_2", std::move(second));
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
