// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__PIPELINE__PIPELINE_FACTORY_NODE_HPP_
#define PCL_FILTER_COMPONENTS__PIPELINE__PIPELINE_FACTORY_NODE_HPP_

#include <memory>
#include <string>
#include <vector>

#include <rclcpp/executor.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_components/component_manager.hpp>
#include <rclcpp_components/node_instance_wrapper.hpp>

#include "pcl_filter_components/pipeline/pipeline_graph.hpp"

namespace pcl_filter_components::pipeline
{

class PipelineFactoryNode : public rclcpp_lifecycle::LifecycleNode
{
public:
  explicit PipelineFactoryNode(
    std::weak_ptr<rclcpp::Executor> executor,
    const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  using CallbackReturn =
    rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

protected:
  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;

private:
  struct LoadedComponent
  {
    std::string id;
    rclcpp_components::NodeInstanceWrapper wrapper;
  };

  void loadPipeline();
  void unloadPipeline();
  std::vector<rclcpp::Parameter> parametersForNode(const PipelineNode & node) const;
  std::string inputTopicForNode(const std::string & node_id) const;
  std::string outputTopicForNode(const std::string & node_id) const;

  std::weak_ptr<rclcpp::Executor> executor_;
  std::shared_ptr<rclcpp_components::ComponentManager> component_manager_;
  PipelineGraph graph_;
  std::vector<LoadedComponent> loaded_components_;
  std::string pipeline_file_;
};

}  // namespace pcl_filter_components::pipeline

#endif  // PCL_FILTER_COMPONENTS__PIPELINE__PIPELINE_FACTORY_NODE_HPP_
