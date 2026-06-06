// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_

#include <array>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/filters/crop_box_filter.hpp"
#include "pcl_filter_base/ros/parameter_utils.hpp"
#include "pcl_filter_base/ros/pcl_filter_component_base.hpp"

namespace pcl_filter_components::ros
{

using pcl_filter_base::ros::declareParameterIfNotDeclared;
using pcl_filter_base::ros::getParameter;
using pcl_filter_base::ros::makeFloatingPointRangeParameterDescriptor;
using pcl_filter_base::ros::makeParameterDescriptor;
using pcl_filter_base::ros::PclFilterComponentBase;

template <typename PointT>
class CropBoxComponent
  : public PclFilterComponentBase<PointT, filters::CropBoxFilter<PointT>>
{
public:
  using Base = PclFilterComponentBase<PointT, filters::CropBoxFilter<PointT>>;
  using CloudAdapter = typename Base::CloudAdapter;
  using IndicesAdapter = typename Base::IndicesAdapter;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = typename Base::StampedCloud;

  explicit CropBoxComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(
      "crop_box_filter",
      options,
      inputPorts(),
      outputPorts())
  {
    declareParameterIfNotDeclared(
      *this,
      "filter.min_x",
      -10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum x bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.min_y",
      -10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum y bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.min_z",
      -2.0,
      makeFloatingPointRangeParameterDescriptor(
        "Minimum z bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_x",
      10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum x bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_y",
      10.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum y bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.max_z",
      3.0,
      makeFloatingPointRangeParameterDescriptor(
        "Maximum z bound of the crop box in meters.",
        -1.0e6,
        1.0e6));
    declareParameterIfNotDeclared(
      *this,
      "filter.invert",
      false,
      makeParameterDescriptor("Keep points outside the crop box when enabled."));
    declareParameterIfNotDeclared(
      *this,
      "filter.output_indices",
      false,
      makeParameterDescriptor("Publish filtered point indices instead of a filtered point cloud."));
  }

protected:
  static std::array<PortDescriptor, 1> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>(
        "cloud",
        "/points/input",
        "Input point cloud topic."),
    }};
  }

  static std::array<PortDescriptor, 3> outputPorts()
  {
    return {{
      Base::template outputPort<CloudAdapter>(
        "cloud",
        "/points/output",
        "Filtered point cloud topic."),
      Base::template outputPort<IndicesAdapter>(
        "indices",
        "/points/indices",
        "Filtered point indices topic."),
      Base::template outputPort<CloudAdapter>(
        "orig_cloud",
        "/points/original",
        "Original input point cloud topic."),
    }};
  }

  void configureFilter() override
  {
    typename filters::CropBoxFilter<PointT>::Params params;
    params.min_x = getParameter<double>(*this, "filter.min_x");
    params.min_y = getParameter<double>(*this, "filter.min_y");
    params.min_z = getParameter<double>(*this, "filter.min_z");
    params.max_x = getParameter<double>(*this, "filter.max_x");
    params.max_y = getParameter<double>(*this, "filter.max_y");
    params.max_z = getParameter<double>(*this, "filter.max_z");
    params.invert = getParameter<bool>(*this, "filter.invert");
    output_indices_ = getParameter<bool>(*this, "filter.output_indices");
    this->filter_.configure(params);
  }

  void processCloud(std::unique_ptr<StampedCloud> input) override
  {
    if (output_indices_) {
      this->publishFilterIndices("indices", *input);
      this->publishCloud("orig_cloud", std::move(input));
      return;
    }
    auto output = std::make_unique<StampedCloud>();
    output->header = input->header;
    this->filter_.filter(*input, *output);
    this->publishCloud(std::move(output));
    this->publishCloud("orig_cloud", std::move(input));
  }

  bool output_indices_{false};
};

}  // namespace pcl_filter_components::ros

#endif  // PCL_FILTER_COMPONENTS__ROS__CROP_BOX_COMPONENT_HPP_
