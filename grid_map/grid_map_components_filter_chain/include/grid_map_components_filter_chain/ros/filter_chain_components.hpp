// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef GRID_MAP_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_
#define GRID_MAP_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_

#include <grid_map_core/grid_map_core.hpp>

#include "filter_component_base/ros/filter_chain_component.hpp"
#include "grid_map_components_type_adapter/ros/grid_map_type_adapter.hpp"

namespace grid_map_components_filter_chain::ros
{

using GridMapAdapter =
  grid_map_components_type_adapter::ros::GridMapAdapter;

struct RosFilterChainGridMapTraits
{
  static const char * nodeName() {return "ros_filter_chain_grid_map";}
  static const char * dataType() {return "grid_map::GridMap";}
  static const char * inputPort() {return "map";}
  static const char * outputPort() {return "map";}
  static const char * originalInputPort() {return "orig_input";}
};

using RosFilterChainGridMapComponent =
  filter_component_base::ros::FilterChainComponent<
  GridMapAdapter,
  RosFilterChainGridMapTraits>;

}  // namespace grid_map_components_filter_chain::ros

#endif  // GRID_MAP_COMPONENTS_FILTER_CHAIN__ROS__FILTER_CHAIN_COMPONENTS_HPP_
