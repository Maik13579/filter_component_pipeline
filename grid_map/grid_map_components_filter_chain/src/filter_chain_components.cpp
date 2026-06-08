// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <rclcpp_components/register_node_macro.hpp>

#include "grid_map_components_filter_chain/ros/filter_chain_components.hpp"

namespace grid_map_components_filter_chain
{

using RosFilterChainGridMapComponent = ros::RosFilterChainGridMapComponent;

}  // namespace grid_map_components_filter_chain

RCLCPP_COMPONENTS_REGISTER_NODE(grid_map_components_filter_chain::RosFilterChainGridMapComponent)
