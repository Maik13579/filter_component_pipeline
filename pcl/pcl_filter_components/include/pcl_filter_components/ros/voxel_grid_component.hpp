// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/voxel_grid_filter.hpp"
#include "filter_component_base/ros/parameter_utils.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros
{

using filter_component_base::ros::declareParameterIfNotDeclared;
using filter_component_base::ros::getParameter;
using filter_component_base::ros::makeFloatingPointRangeParameterDescriptor;
using filter_component_base::ros::makeParameterDescriptor;
using filter_component_base::ros::FilterComponentBase;

template <typename PointT>
class VoxelGridComponent
  : public FilterComponentBase
{
public:
  using Filter = filters::VoxelGridFilter<PointT>;
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit VoxelGridComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "voxel_grid_filter",
      options,
      inputPorts(),
      outputPorts())
  {
    declareParameterIfNotDeclared(
      *this,
      "filter.leaf_size_x",
      0.05,
      makeFloatingPointRangeParameterDescriptor(
        "Voxel grid leaf size along the x axis in meters.",
        1.0e-6,
        1000.0));
    declareParameterIfNotDeclared(
      *this,
      "filter.leaf_size_y",
      0.05,
      makeFloatingPointRangeParameterDescriptor(
        "Voxel grid leaf size along the y axis in meters.",
        1.0e-6,
        1000.0));
    declareParameterIfNotDeclared(
      *this,
      "filter.leaf_size_z",
      0.05,
      makeFloatingPointRangeParameterDescriptor(
        "Voxel grid leaf size along the z axis in meters.",
        1.0e-6,
        1000.0));
    declareParameterIfNotDeclared(
      *this,
      "filter.invert",
      false,
      makeParameterDescriptor("Return points outside the selected voxel representatives."));
  }

protected:
  static std::array<PortDescriptor, 1> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>(
        "cloud",
        "Input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 2> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "Filtered point cloud topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_cloud",
        "Original input point cloud topic."),
    }};
  }

  void configure() override
  {
    typename Filter::Params params;
    params.leaf_size_x = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_x"));
    params.leaf_size_y = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_y"));
    params.leaf_size_z = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_z"));
    params.invert = getParameter<bool>(*this, "filter.invert");
    filter_.configure(params);
  }

  void process() override
  {
    auto input = this->template takeInput<CloudAdapter>("cloud");
    if (!input) {
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    filter_.filter(*input, *output);
    this->template publish<CloudAdapter>("cloud", std::move(output));
    this->template publish<CloudAdapter>("orig_cloud", std::move(input));
  }

  Filter filter_;
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_
