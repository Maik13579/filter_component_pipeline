# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pipeline_file = LaunchConfiguration("pipeline_file")

    factory_node = Node(
        package="filter_component_factory",
        executable="pcl_pipeline_factory",
        name="pcl_pipeline_factory",
        output="screen",
        parameters=[{"pipeline_file": pipeline_file}],
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            "pipeline_file",
            default_value=PathJoinSubstitution([
                FindPackageShare("filter_component_factory"),
                "config",
                "example_pipeline.yaml",
            ]),
            description="Path to the saved PCL filter pipeline YAML graph.",
        ),
        factory_node,
    ])
