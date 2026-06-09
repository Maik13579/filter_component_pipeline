# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from typing import Any

from rcl_interfaces.msg import ParameterDescriptor

from filter_component_base_py import FilterComponentBasePy
from filter_component_base_py import input_port
from filter_component_base_py import output_port
from filter_component_base_py.adapters import PointCloud2NumpyAdapter


class PythonPointCloudPassthrough(FilterComponentBasePy):
    def __init__(self, node_name: str = "python_point_cloud_passthrough", **kwargs: Any) -> None:
        super().__init__(
            node_name,
            input_ports=[
                input_port("cloud", PointCloud2NumpyAdapter, "Input PointCloud2 stream."),
            ],
            output_ports=[
                output_port("cloud", PointCloud2NumpyAdapter, "Output PointCloud2 stream."),
            ],
            **kwargs,
        )
        self.declare_parameter(
            "filter.enabled",
            True,
            ParameterDescriptor(description="Forward incoming clouds when enabled."),
        )
        self.declare_parameter(
            "filter.frame_id_override",
            "",
            ParameterDescriptor(description="Optional output frame id override."),
        )

    def process(self) -> None:
        cloud = self.take_input("cloud")
        if cloud is None or not self.get_parameter("filter.enabled").value:
            return
        frame_id = self.get_parameter("filter.frame_id_override").value
        if frame_id:
            cloud.header.frame_id = frame_id
        self.publish("cloud", cloud)
