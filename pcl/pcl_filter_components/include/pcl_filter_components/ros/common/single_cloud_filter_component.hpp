// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__COMMON__SINGLE_CLOUD_FILTER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__COMMON__SINGLE_CLOUD_FILTER_COMPONENT_HPP_

#include <array>
#include <memory>
#include <string>
#include <type_traits>

#include <rclcpp/rclcpp.hpp>

#include "filter_component_base/ros/parameter_utils.hpp"
#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros::common
{

using filter_component_base::ros::declareParameterIfNotDeclared;
using filter_component_base::ros::getParameter;
using filter_component_base::ros::makeFloatingPointRangeParameterDescriptor;
using filter_component_base::ros::makeIntegerRangeParameterDescriptor;
using filter_component_base::ros::makeParameterDescriptor;
using filter_component_base::ros::FilterComponentBase;

namespace detail
{

template <typename FilterT, typename = void>
struct HasInvertParam : std::false_type {};
template <typename FilterT>
struct HasInvertParam<FilterT, std::void_t<decltype(std::declval<typename FilterT::Params>().invert)> > : std::true_type {};

template <typename NodeT>
void declareInvert(NodeT & node)
{
  declareParameterIfNotDeclared(node, "filter.invert", false, makeParameterDescriptor("Invert the selected points."));
}

template <typename FilterT, typename NodeT>
void declareParams(NodeT & node)
{
  typename FilterT::Params params{};
  if constexpr (requires { params.leaf_size_x; })
  {
    declareParameterIfNotDeclared(node, "filter.leaf_size_x", 0.05, makeFloatingPointRangeParameterDescriptor("Leaf size along x in meters.", 1.0e-6, 1000.0));
    declareParameterIfNotDeclared(node, "filter.leaf_size_y", 0.05, makeFloatingPointRangeParameterDescriptor("Leaf size along y in meters.", 1.0e-6, 1000.0));
    declareParameterIfNotDeclared(node, "filter.leaf_size_z", 0.05, makeFloatingPointRangeParameterDescriptor("Leaf size along z in meters.", 1.0e-6, 1000.0));
  }
  if constexpr (requires { params.radius; })
  {
    declareParameterIfNotDeclared(node, "filter.radius", 0.25, makeFloatingPointRangeParameterDescriptor("Radius in meters.", 1.0e-6, 1000.0));
  }
  if constexpr (requires { params.sample_size; })
  {
    declareParameterIfNotDeclared(node, "filter.sample_size", 1000, makeIntegerRangeParameterDescriptor("Number of sampled points.", 0, 100000000));
  }
  if constexpr (requires { params.resolution; })
  {
    declareParameterIfNotDeclared(node, "filter.resolution", 0.05, makeFloatingPointRangeParameterDescriptor("Grid or morphology resolution in meters.", 1.0e-6, 1000.0));
  }
  if constexpr (requires { params.mean_k; })
  {
    declareParameterIfNotDeclared(node, "filter.mean_k", 20, makeIntegerRangeParameterDescriptor("Neighbors used for statistics.", 1, 100000));
    declareParameterIfNotDeclared(node, "filter.stddev", 1.0, makeFloatingPointRangeParameterDescriptor("Standard deviation multiplier threshold.", 0.0, 1000.0));
  }
  if constexpr (requires { params.min_neighbors; })
  {
    declareParameterIfNotDeclared(node, "filter.min_neighbors", 2, makeIntegerRangeParameterDescriptor("Minimum neighbors inside radius.", 0, 100000));
  }
  if constexpr (requires { params.field_name; })
  {
    declareParameterIfNotDeclared(node, "filter.field_name", std::string{"z"}, makeParameterDescriptor("Point field used by the filter."));
    declareParameterIfNotDeclared(node, "filter.min_value", -1.0, makeFloatingPointRangeParameterDescriptor("Minimum accepted field value.", -1.0e9, 1.0e9));
    declareParameterIfNotDeclared(node, "filter.max_value", 2.0, makeFloatingPointRangeParameterDescriptor("Maximum accepted field value.", -1.0e9, 1.0e9));
  }
  if constexpr (requires { params.start_index; })
  {
    declareParameterIfNotDeclared(node, "filter.start_index", 0, makeIntegerRangeParameterDescriptor("First selected point index.", 0, 100000000));
    declareParameterIfNotDeclared(node, "filter.count", 1000000, makeIntegerRangeParameterDescriptor("Number of selected point indices.", 0, 100000000));
    declareParameterIfNotDeclared(node, "filter.keep_organized", false, makeParameterDescriptor("Keep cloud organized when extracting points."));
  }
  if constexpr (requires { params.horizontal_fov; })
  {
    declareParameterIfNotDeclared(node, "filter.horizontal_fov", 90.0, makeFloatingPointRangeParameterDescriptor("Horizontal field of view in degrees.", 1.0, 179.0));
    declareParameterIfNotDeclared(node, "filter.vertical_fov", 60.0, makeFloatingPointRangeParameterDescriptor("Vertical field of view in degrees.", 1.0, 179.0));
    declareParameterIfNotDeclared(node, "filter.near_plane", 0.0, makeFloatingPointRangeParameterDescriptor("Near plane distance in meters.", 0.0, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.far_plane", 100.0, makeFloatingPointRangeParameterDescriptor("Far plane distance in meters.", 1.0e-6, 1.0e6));
  }
  if constexpr (requires { params.center_x; })
  {
    declareParameterIfNotDeclared(node, "filter.center_x", 0.0, makeFloatingPointRangeParameterDescriptor("Sphere center x in meters.", -1.0e6, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.center_y", 0.0, makeFloatingPointRangeParameterDescriptor("Sphere center y in meters.", -1.0e6, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.center_z", 0.0, makeFloatingPointRangeParameterDescriptor("Sphere center z in meters.", -1.0e6, 1.0e6));
  }
  if constexpr (requires { params.a; })
  {
    declareParameterIfNotDeclared(node, "filter.a", 0.0, makeFloatingPointRangeParameterDescriptor("Plane coefficient a.", -1.0e6, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.b", 0.0, makeFloatingPointRangeParameterDescriptor("Plane coefficient b.", -1.0e6, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.c", 1.0, makeFloatingPointRangeParameterDescriptor("Plane coefficient c.", -1.0e6, 1.0e6));
    declareParameterIfNotDeclared(node, "filter.d", 0.0, makeFloatingPointRangeParameterDescriptor("Plane coefficient d.", -1.0e6, 1.0e6));
  }
  if constexpr (requires { params.threshold; })
  {
    declareParameterIfNotDeclared(node, "filter.threshold", 0.05, makeFloatingPointRangeParameterDescriptor("Distance threshold in meters.", 0.0, 1000.0));
  }
  if constexpr (requires { params.distance_threshold; })
  {
    declareParameterIfNotDeclared(node, "filter.distance_threshold", 0.05, makeFloatingPointRangeParameterDescriptor("SAC distance threshold in meters.", 0.0, 1000.0));
    declareParameterIfNotDeclared(node, "filter.max_iterations", 100, makeIntegerRangeParameterDescriptor("Maximum SAC iterations.", 1, 1000000));
    declareParameterIfNotDeclared(node, "filter.optimize_coefficients", true, makeParameterDescriptor("Refine fitted model coefficients."));
  }
  if constexpr (requires { params.cluster_tolerance; })
  {
    declareParameterIfNotDeclared(node, "filter.cluster_tolerance", 0.2, makeFloatingPointRangeParameterDescriptor("Cluster tolerance in meters.", 1.0e-6, 1000.0));
    declareParameterIfNotDeclared(node, "filter.min_cluster_size", 1, makeIntegerRangeParameterDescriptor("Minimum cluster size.", 1, 100000000));
    declareParameterIfNotDeclared(node, "filter.max_cluster_size", 1000000, makeIntegerRangeParameterDescriptor("Maximum cluster size.", 1, 100000000));
    declareParameterIfNotDeclared(node, "filter.cluster_index", 0, makeIntegerRangeParameterDescriptor("Cluster index after size sort.", 0, 1000000));
  }
  if constexpr (requires { params.window_size; })
  {
    declareParameterIfNotDeclared(node, "filter.window_size", 5, makeIntegerRangeParameterDescriptor("Median filter window size.", 1, 100000));
    declareParameterIfNotDeclared(node, "filter.max_allowed_movement", 1.0e9, makeFloatingPointRangeParameterDescriptor("Maximum allowed z movement.", 0.0, 1.0e9));
  }
  if constexpr (requires { params.half_size; })
  {
    declareParameterIfNotDeclared(node, "filter.half_size", 0.05, makeFloatingPointRangeParameterDescriptor("Bilateral spatial half size.", 1.0e-6, 1000.0));
    declareParameterIfNotDeclared(node, "filter.stddev", 0.03, makeFloatingPointRangeParameterDescriptor("Bilateral range standard deviation.", 1.0e-6, 1000.0));
  }
  if constexpr (requires { params.min_points_per_voxel; })
  {
    declareParameterIfNotDeclared(node, "filter.min_points_per_voxel", 3, makeIntegerRangeParameterDescriptor("Minimum points per covariance voxel.", 3, 1000000));
  }
  if constexpr (requires { params.operation; })
  {
    declareParameterIfNotDeclared(node, "filter.operation", 0, makeIntegerRangeParameterDescriptor("Morphological operation enum: open=0, close=1, dilate=2, erode=3.", 0, 3));
  }
  if constexpr (requires { params.search_radius; })
  {
    declareParameterIfNotDeclared(node, "filter.search_radius", 0.03, makeFloatingPointRangeParameterDescriptor("MLS search radius in meters.", 1.0e-6, 1000.0));
    declareParameterIfNotDeclared(node, "filter.polynomial_order", 2, makeIntegerRangeParameterDescriptor("MLS polynomial order.", 0, 10));
  }
  if constexpr (requires { params.min_r; })
  {
    for (const auto & name : {"min_r", "min_g", "min_b"})
    {
      declareParameterIfNotDeclared(node, std::string{"filter."} + name, 0, makeIntegerRangeParameterDescriptor("Minimum accepted color channel value.", 0, 255));
    }
    for (const auto & name : {"max_r", "max_g", "max_b"})
    {
      declareParameterIfNotDeclared(node, std::string{"filter."} + name, 255, makeIntegerRangeParameterDescriptor("Maximum accepted color channel value.", 0, 255));
    }
  }
  if constexpr (requires { params.min_alpha; })
  {
    declareParameterIfNotDeclared(node, "filter.min_alpha", 1, makeIntegerRangeParameterDescriptor("Minimum alpha value.", 0, 255));
    declareParameterIfNotDeclared(node, "filter.max_alpha", 255, makeIntegerRangeParameterDescriptor("Maximum alpha value.", 0, 255));
  }
  if constexpr (requires { params.min_intensity; })
  {
    declareParameterIfNotDeclared(node, "filter.min_intensity", 0.0, makeFloatingPointRangeParameterDescriptor("Minimum intensity.", -1.0e9, 1.0e9));
    declareParameterIfNotDeclared(node, "filter.max_intensity", 1.0e9, makeFloatingPointRangeParameterDescriptor("Maximum intensity.", -1.0e9, 1.0e9));
  }
  if constexpr (HasInvertParam<FilterT>::value)
  {
    declareInvert(node);
  }
}

template <typename FilterT, typename NodeT>
typename FilterT::Params readParams(NodeT & node)
{
  typename FilterT::Params params{};
  if constexpr (requires { params.leaf_size_x; })
  {
    params.leaf_size_x = static_cast<float>(
      getParameter<double>(node, "filter.leaf_size_x"));
    params.leaf_size_y = static_cast<float>(
      getParameter<double>(node, "filter.leaf_size_y"));
    params.leaf_size_z = static_cast<float>(
      getParameter<double>(node, "filter.leaf_size_z"));
  }
  if constexpr (requires { params.radius; })
  {
    params.radius = getParameter<double>(node, "filter.radius");
  }
  if constexpr (requires { params.sample_size; })
  {
    params.sample_size = getParameter<int>(node, "filter.sample_size");
  }
  if constexpr (requires { params.resolution; })
  {
    params.resolution = getParameter<double>(node, "filter.resolution");
  }
  if constexpr (requires { params.mean_k; })
  {
    params.mean_k = getParameter<int>(node, "filter.mean_k");
    params.stddev = getParameter<double>(node, "filter.stddev");
  }
  if constexpr (requires { params.min_neighbors; })
  {
    params.min_neighbors = getParameter<int>(node, "filter.min_neighbors");
  }
  if constexpr (requires { params.field_name; })
  {
    params.field_name = getParameter<std::string>(node, "filter.field_name");
    params.min_value = getParameter<double>(node, "filter.min_value");
    params.max_value = getParameter<double>(node, "filter.max_value");
  }
  if constexpr (requires { params.start_index; })
  {
    params.start_index = getParameter<int>(node, "filter.start_index");
    params.count = getParameter<int>(node, "filter.count");
    params.keep_organized = getParameter<bool>(node, "filter.keep_organized");
  }
  if constexpr (requires { params.horizontal_fov; })
  {
    params.horizontal_fov = getParameter<double>(node, "filter.horizontal_fov");
    params.vertical_fov = getParameter<double>(node, "filter.vertical_fov");
    params.near_plane = getParameter<double>(node, "filter.near_plane");
    params.far_plane = getParameter<double>(node, "filter.far_plane");
  }
  if constexpr (requires { params.center_x; })
  {
    params.center_x = getParameter<double>(node, "filter.center_x");
    params.center_y = getParameter<double>(node, "filter.center_y");
    params.center_z = getParameter<double>(node, "filter.center_z");
  }
  if constexpr (requires { params.a; })
  {
    params.a = getParameter<double>(node, "filter.a");
    params.b = getParameter<double>(node, "filter.b");
    params.c = getParameter<double>(node, "filter.c");
    params.d = getParameter<double>(node, "filter.d");
  }
  if constexpr (requires { params.threshold; })
  {
    params.threshold = getParameter<double>(node, "filter.threshold");
  }
  if constexpr (requires { params.distance_threshold; })
  {
    params.distance_threshold = getParameter<double>(node, "filter.distance_threshold");
    params.max_iterations = getParameter<int>(node, "filter.max_iterations");
    params.optimize_coefficients = getParameter<bool>(node, "filter.optimize_coefficients");
  }
  if constexpr (requires { params.cluster_tolerance; })
  {
    params.cluster_tolerance = getParameter<double>(node, "filter.cluster_tolerance");
    params.min_cluster_size = getParameter<int>(node, "filter.min_cluster_size");
    params.max_cluster_size = getParameter<int>(node, "filter.max_cluster_size");
    params.cluster_index = getParameter<int>(node, "filter.cluster_index");
  }
  if constexpr (requires { params.window_size; })
  {
    params.window_size = getParameter<int>(node, "filter.window_size");
    params.max_allowed_movement = getParameter<double>(node, "filter.max_allowed_movement");
  }
  if constexpr (requires { params.half_size; })
  {
    params.half_size = getParameter<double>(node, "filter.half_size");
    params.stddev = getParameter<double>(node, "filter.stddev");
  }
  if constexpr (requires { params.min_points_per_voxel; })
  {
    params.min_points_per_voxel = getParameter<int>(node, "filter.min_points_per_voxel");
  }
  if constexpr (requires { params.operation; })
  {
    params.operation = getParameter<int>(node, "filter.operation");
  }
  if constexpr (requires { params.search_radius; })
  {
    params.search_radius = getParameter<double>(node, "filter.search_radius");
    params.polynomial_order = getParameter<int>(node, "filter.polynomial_order");
  }
  if constexpr (requires { params.min_r; })
  {
    params.min_r = getParameter<int>(node, "filter.min_r");
    params.min_g = getParameter<int>(node, "filter.min_g");
    params.min_b = getParameter<int>(node, "filter.min_b");
    params.max_r = getParameter<int>(node, "filter.max_r");
    params.max_g = getParameter<int>(node, "filter.max_g");
    params.max_b = getParameter<int>(node, "filter.max_b");
  }
  if constexpr (requires { params.min_alpha; })
  {
    params.min_alpha = getParameter<int>(node, "filter.min_alpha");
    params.max_alpha = getParameter<int>(node, "filter.max_alpha");
  }
  if constexpr (requires { params.min_intensity; })
  {
    params.min_intensity = getParameter<double>(node, "filter.min_intensity");
    params.max_intensity = getParameter<double>(node, "filter.max_intensity");
  }
  if constexpr (HasInvertParam<FilterT>::value)
  {
    params.invert = getParameter<bool>(node, "filter.invert");
  }
  return params;
}

}  // namespace detail

template <typename PointT, typename FilterT>
class SingleCloudFilterComponent : public FilterComponentBase
{
public:
  using Base = FilterComponentBase;
  using CloudAdapter = pcl_filter_components_type_adapters::ros::PclCloudAdapter<PointT>;
  using PortDescriptor = typename Base::PortDescriptor;
  using StampedCloud = pcl::PointCloud<PointT>;

  explicit SingleCloudFilterComponent(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Base(FilterT::nodeName(), options, inputPorts(), outputPorts())
  {
    detail::declareParams<FilterT>(*this);
  }

protected:
  static std::array<PortDescriptor, 1> inputPorts()
  {
    return {{
      Base::template inputPort<CloudAdapter>("cloud", "Input point cloud topic."),
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
    filter_.configure(detail::readParams<FilterT>(*this));
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

  FilterT filter_;
};

}  // namespace pcl_filter_components::ros::common

#endif  // PCL_FILTER_COMPONENTS__ROS__COMMON__SINGLE_CLOUD_FILTER_COMPONENT_HPP_
