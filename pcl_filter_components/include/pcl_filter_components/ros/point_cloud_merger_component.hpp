// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_

#include <memory>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_base/ros/pcl_filter_component_base.hpp"
#include "pcl_filter_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

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
  : public pcl_filter_base::ros::PclFilterComponentBase<PointT, detail::NoopFilter<PointT>>
{
public:
  using Base = pcl_filter_base::ros::PclFilterComponentBase<PointT, detail::NoopFilter<PointT>>;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = typename Base::StampedCloud;
  using CloudAdapter = typename Base::CloudAdapter;

  explicit PointCloudMergerComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "point_cloud_merger",
      options,
      inputPorts(),
      outputPorts())
  {
  }

protected:
  static std::vector<PortDescriptor> inputPorts()
  {
    return {
      {"input_1", "/points/input_a", "First input point cloud topic."},
      {"input_2", "/points/input_b", "Second input point cloud topic."},
    };
  }

  static std::vector<PortDescriptor> outputPorts()
  {
    return {{"cloud", "/points/output", "Merged output point cloud topic."}};
  }

  void configureFilter() override
  {
  }

  void processInputs() override
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
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__POINT_CLOUD_MERGER_COMPONENT_HPP_
