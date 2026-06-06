// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/voxel_grid_filter.hpp"
#include "pcl_filter_components/ros/optional_indices_output_component.hpp"
#include "pcl_filter_components/ros/parameter_utils.hpp"

namespace pcl_filter_components::ros
{

template <typename PointT>
class VoxelGridComponent
  : public OptionalIndicesOutputComponent<PointT, filters::VoxelGridFilter<PointT>>
{
public:
  explicit VoxelGridComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : OptionalIndicesOutputComponent<PointT, filters::VoxelGridFilter<PointT>>(
      "voxel_grid_filter",
      options)
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
      makeParameterDescriptor("Return points or indices outside the selected voxel representatives."));
  }

protected:
  void configureFilter() override
  {
    typename filters::VoxelGridFilter<PointT>::Params params;
    params.leaf_size_x = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_x"));
    params.leaf_size_y = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_y"));
    params.leaf_size_z = static_cast<float>(getParameter<double>(*this, "filter.leaf_size_z"));
    params.invert = getParameter<bool>(*this, "filter.invert");
    this->filter_.configure(params);
  }
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__VOXEL_GRID_COMPONENT_HPP_
