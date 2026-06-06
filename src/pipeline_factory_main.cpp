// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <string>

#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

#include "pcl_filter_components/pipeline/pipeline_factory_node.hpp"

namespace
{

size_t executorThreadsFromArguments(int argc, char ** argv)
{
  const auto prefix = std::string{"executor_threads:="};
  for (int index = 1; index < argc; ++index) {
    const auto argument = std::string{argv[index]};
    const auto position = argument.find(prefix);
    if (position == std::string::npos) {
      continue;
    }
    const auto value = argument.substr(position + prefix.size());
    return static_cast<size_t>(std::stoul(value));
  }
  return 0U;
}

}  // namespace

int main(int argc, char ** argv)
{
  const auto executor_threads = executorThreadsFromArguments(argc, argv);
  rclcpp::init(argc, argv);
  std::shared_ptr<rclcpp::Executor> executor;
  if (executor_threads > 1U) {
    executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>(
      rclcpp::ExecutorOptions{},
      executor_threads,
      false);
  } else {
    executor = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  }
  auto node = std::make_shared<pcl_filter_components::pipeline::PipelineFactoryNode>(
    executor,
    rclcpp::NodeOptions{}
    .use_intra_process_comms(true)
    .parameter_overrides({rclcpp::Parameter{"executor_threads", static_cast<int>(executor_threads)}}));
  executor->add_node(node->get_node_base_interface());
  executor->spin();
  rclcpp::shutdown();
  return 0;
}
