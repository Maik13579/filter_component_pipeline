// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <memory>

#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/pipeline/pipeline_factory_node.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  auto node = std::make_shared<pcl_filter_components::pipeline::PipelineFactoryNode>(
    executor,
    rclcpp::NodeOptions{}.use_intra_process_comms(true));
  executor->add_node(node->get_node_base_interface());
  executor->spin();
  rclcpp::shutdown();
  return 0;
}
