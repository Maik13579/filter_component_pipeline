// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__ROS__COMMON__SINGLE_CLOUD_FILTER_COMPONENT_HPP_
#define PCL_FILTER_COMPONENTS__ROS__COMMON__SINGLE_CLOUD_FILTER_COMPONENT_HPP_

#include <array>
#include <memory>
#include <string>
#include <type_traits>

#include <rclcpp/rclcpp.hpp>

#include "filter_component_base/ros/filter_component_base.hpp"
#include "pcl_filter_components_type_adapters/ros/stamped_pcl_type_adapter.hpp"

namespace pcl_filter_components::ros::common
{

using filter_component_base::ros::FilterComponentBase;
using filter_component_base::ros::PortDescriptor;

namespace detail
{

template <typename FilterT, typename = void>
struct HasInvertParam : std::false_type {};
template <typename FilterT>
struct HasInvertParam<FilterT, std::void_t<decltype(std::declval<typename FilterT::Params>().invert)> > : std::true_type {};

template <typename NodeT>
void declareInvert(NodeT & node)
{
  node.declareParameter("filter.invert", false, "Invert the selected points.");
}

template <typename FilterT, typename NodeT>
void declareParams(NodeT & node)
{
  typename FilterT::Params params{};
  if constexpr (requires { params.leaf_size_x; })
  {
    node.declareParameter("filter.leaf_size_x", 0.05, "Leaf size along x in meters.");
    node.declareParameter("filter.leaf_size_y", 0.05, "Leaf size along y in meters.");
    node.declareParameter("filter.leaf_size_z", 0.05, "Leaf size along z in meters.");
  }
  if constexpr (requires { params.radius; })
  {
    node.declareParameter("filter.radius", 0.25, "Radius in meters.");
  }
  if constexpr (requires { params.sample_size; })
  {
    node.declareParameter("filter.sample_size", 1000, "Number of sampled points.");
  }
  if constexpr (requires { params.resolution; })
  {
    node.declareParameter("filter.resolution", 0.05, "Grid or morphology resolution in meters.");
  }
  if constexpr (requires { params.mean_k; })
  {
    node.declareParameter("filter.mean_k", 20, "Neighbors used for statistics.");
    node.declareParameter("filter.stddev", 1.0, "Standard deviation multiplier threshold.");
  }
  if constexpr (requires { params.min_neighbors; })
  {
    node.declareParameter("filter.min_neighbors", 2, "Minimum neighbors inside radius.");
  }
  if constexpr (requires { params.field_name; })
  {
    node.declareParameter("filter.field_name", std::string{"z"}, "Point field used by the filter.");
    node.declareParameter("filter.min_value", -1.0, "Minimum accepted field value.");
    node.declareParameter("filter.max_value", 2.0, "Maximum accepted field value.");
  }
  if constexpr (requires { params.start_index; })
  {
    node.declareParameter("filter.start_index", 0, "First selected point index.");
    node.declareParameter("filter.count", 1000000, "Number of selected point indices.");
    node.declareParameter("filter.keep_organized", false, "Keep cloud organized when extracting points.");
  }
  if constexpr (requires { params.horizontal_fov; })
  {
    node.declareParameter("filter.horizontal_fov", 90.0, "Horizontal field of view in degrees.");
    node.declareParameter("filter.vertical_fov", 60.0, "Vertical field of view in degrees.");
    node.declareParameter("filter.near_plane", 0.0, "Near plane distance in meters.");
    node.declareParameter("filter.far_plane", 100.0, "Far plane distance in meters.");
  }
  if constexpr (requires { params.center_x; })
  {
    node.declareParameter("filter.center_x", 0.0, "Sphere center x in meters.");
    node.declareParameter("filter.center_y", 0.0, "Sphere center y in meters.");
    node.declareParameter("filter.center_z", 0.0, "Sphere center z in meters.");
  }
  if constexpr (requires { params.a; })
  {
    node.declareParameter("filter.a", 0.0, "Plane coefficient a.");
    node.declareParameter("filter.b", 0.0, "Plane coefficient b.");
    node.declareParameter("filter.c", 1.0, "Plane coefficient c.");
    node.declareParameter("filter.d", 0.0, "Plane coefficient d.");
  }
  if constexpr (requires { params.threshold; })
  {
    node.declareParameter("filter.threshold", 0.05, "Distance threshold in meters.");
  }
  if constexpr (requires { params.distance_threshold; })
  {
    node.declareParameter("filter.distance_threshold", 0.05, "SAC distance threshold in meters.");
    node.declareParameter("filter.max_iterations", 100, "Maximum SAC iterations.");
    node.declareParameter("filter.optimize_coefficients", true, "Refine fitted model coefficients.");
  }
  if constexpr (requires { params.cluster_tolerance; })
  {
    node.declareParameter("filter.cluster_tolerance", 0.2, "Cluster tolerance in meters.");
    node.declareParameter("filter.min_cluster_size", 1, "Minimum cluster size.");
    node.declareParameter("filter.max_cluster_size", 1000000, "Maximum cluster size.");
    node.declareParameter("filter.cluster_index", 0, "Cluster index after size sort.");
  }
  if constexpr (requires { params.window_size; })
  {
    node.declareParameter("filter.window_size", 5, "Median filter window size.");
    node.declareParameter("filter.max_allowed_movement", 1.0e9, "Maximum allowed z movement.");
  }
  if constexpr (requires { params.half_size; })
  {
    node.declareParameter("filter.half_size", 0.05, "Bilateral spatial half size.");
    node.declareParameter("filter.stddev", 0.03, "Bilateral range standard deviation.");
  }
  if constexpr (requires { params.min_points_per_voxel; })
  {
    node.declareParameter("filter.min_points_per_voxel", 3, "Minimum points per covariance voxel.");
  }
  if constexpr (requires { params.operation; })
  {
    node.declareParameter("filter.operation", 0, "Morphological operation enum: open=0, close=1, dilate=2, erode=3.");
  }
  if constexpr (requires { params.search_radius; })
  {
    node.declareParameter("filter.search_radius", 0.03, "MLS search radius in meters.");
    node.declareParameter("filter.polynomial_order", 2, "MLS polynomial order.");
  }
  if constexpr (requires { params.min_r; })
  {
    for (const auto & name : {"min_r", "min_g", "min_b"})
    {
      node.declareParameter(std::string{"filter."} + name, 0, "Minimum accepted color channel value.");
    }
    for (const auto & name : {"max_r", "max_g", "max_b"})
    {
      node.declareParameter(std::string{"filter."} + name, 255, "Maximum accepted color channel value.");
    }
  }
  if constexpr (requires { params.min_alpha; })
  {
    node.declareParameter("filter.min_alpha", 1, "Minimum alpha value.");
    node.declareParameter("filter.max_alpha", 255, "Maximum alpha value.");
  }
  if constexpr (requires { params.min_intensity; })
  {
    node.declareParameter("filter.min_intensity", 0.0, "Minimum intensity.");
    node.declareParameter("filter.max_intensity", 1.0e9, "Maximum intensity.");
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
      node.template getParameter<double>("filter.leaf_size_x"));
    params.leaf_size_y = static_cast<float>(
      node.template getParameter<double>("filter.leaf_size_y"));
    params.leaf_size_z = static_cast<float>(
      node.template getParameter<double>("filter.leaf_size_z"));
  }
  if constexpr (requires { params.radius; })
  {
    params.radius = node.template getParameter<double>("filter.radius");
  }
  if constexpr (requires { params.sample_size; })
  {
    params.sample_size = node.template getParameter<int>("filter.sample_size");
  }
  if constexpr (requires { params.resolution; })
  {
    params.resolution = node.template getParameter<double>("filter.resolution");
  }
  if constexpr (requires { params.mean_k; })
  {
    params.mean_k = node.template getParameter<int>("filter.mean_k");
    params.stddev = node.template getParameter<double>("filter.stddev");
  }
  if constexpr (requires { params.min_neighbors; })
  {
    params.min_neighbors = node.template getParameter<int>("filter.min_neighbors");
  }
  if constexpr (requires { params.field_name; })
  {
    params.field_name = node.template getParameter<std::string>("filter.field_name");
    params.min_value = node.template getParameter<double>("filter.min_value");
    params.max_value = node.template getParameter<double>("filter.max_value");
  }
  if constexpr (requires { params.start_index; })
  {
    params.start_index = node.template getParameter<int>("filter.start_index");
    params.count = node.template getParameter<int>("filter.count");
    params.keep_organized = node.template getParameter<bool>("filter.keep_organized");
  }
  if constexpr (requires { params.horizontal_fov; })
  {
    params.horizontal_fov = node.template getParameter<double>("filter.horizontal_fov");
    params.vertical_fov = node.template getParameter<double>("filter.vertical_fov");
    params.near_plane = node.template getParameter<double>("filter.near_plane");
    params.far_plane = node.template getParameter<double>("filter.far_plane");
  }
  if constexpr (requires { params.center_x; })
  {
    params.center_x = node.template getParameter<double>("filter.center_x");
    params.center_y = node.template getParameter<double>("filter.center_y");
    params.center_z = node.template getParameter<double>("filter.center_z");
  }
  if constexpr (requires { params.a; })
  {
    params.a = node.template getParameter<double>("filter.a");
    params.b = node.template getParameter<double>("filter.b");
    params.c = node.template getParameter<double>("filter.c");
    params.d = node.template getParameter<double>("filter.d");
  }
  if constexpr (requires { params.threshold; })
  {
    params.threshold = node.template getParameter<double>("filter.threshold");
  }
  if constexpr (requires { params.distance_threshold; })
  {
    params.distance_threshold = node.template getParameter<double>("filter.distance_threshold");
    params.max_iterations = node.template getParameter<int>("filter.max_iterations");
    params.optimize_coefficients = node.template getParameter<bool>("filter.optimize_coefficients");
  }
  if constexpr (requires { params.cluster_tolerance; })
  {
    params.cluster_tolerance = node.template getParameter<double>("filter.cluster_tolerance");
    params.min_cluster_size = node.template getParameter<int>("filter.min_cluster_size");
    params.max_cluster_size = node.template getParameter<int>("filter.max_cluster_size");
    params.cluster_index = node.template getParameter<int>("filter.cluster_index");
  }
  if constexpr (requires { params.window_size; })
  {
    params.window_size = node.template getParameter<int>("filter.window_size");
    params.max_allowed_movement = node.template getParameter<double>("filter.max_allowed_movement");
  }
  if constexpr (requires { params.half_size; })
  {
    params.half_size = node.template getParameter<double>("filter.half_size");
    params.stddev = node.template getParameter<double>("filter.stddev");
  }
  if constexpr (requires { params.min_points_per_voxel; })
  {
    params.min_points_per_voxel = node.template getParameter<int>("filter.min_points_per_voxel");
  }
  if constexpr (requires { params.operation; })
  {
    params.operation = node.template getParameter<int>("filter.operation");
  }
  if constexpr (requires { params.search_radius; })
  {
    params.search_radius = node.template getParameter<double>("filter.search_radius");
    params.polynomial_order = node.template getParameter<int>("filter.polynomial_order");
  }
  if constexpr (requires { params.min_r; })
  {
    params.min_r = node.template getParameter<int>("filter.min_r");
    params.min_g = node.template getParameter<int>("filter.min_g");
    params.min_b = node.template getParameter<int>("filter.min_b");
    params.max_r = node.template getParameter<int>("filter.max_r");
    params.max_g = node.template getParameter<int>("filter.max_g");
    params.max_b = node.template getParameter<int>("filter.max_b");
  }
  if constexpr (requires { params.min_alpha; })
  {
    params.min_alpha = node.template getParameter<int>("filter.min_alpha");
    params.max_alpha = node.template getParameter<int>("filter.max_alpha");
  }
  if constexpr (requires { params.min_intensity; })
  {
    params.min_intensity = node.template getParameter<double>("filter.min_intensity");
    params.max_intensity = node.template getParameter<double>("filter.max_intensity");
  }
  if constexpr (HasInvertParam<FilterT>::value)
  {
    params.invert = node.template getParameter<bool>("filter.invert");
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
