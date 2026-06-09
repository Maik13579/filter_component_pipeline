# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from typing import Any

from filter_component_base_py import FilterComponentBasePy
from filter_component_base_py import declare_parameter_if_not_declared
from filter_component_base_py import get_parameter
from filter_component_base_py import input_port
from filter_component_base_py import make_parameter_descriptor
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
        declare_parameter_if_not_declared(
            self,
            "filter.enabled",
            True,
            make_parameter_descriptor("Forward incoming clouds when enabled."),
        )
        declare_parameter_if_not_declared(
            self,
            "filter.frame_id_override",
            "",
            make_parameter_descriptor("Optional output frame id override."),
        )
        self._enabled = True
        self._frame_id_override = ""

    def configure_filter(self) -> None:
        self._enabled = bool(get_parameter(self, "filter.enabled"))
        self._frame_id_override = str(get_parameter(self, "filter.frame_id_override"))

    def process(self) -> None:
        cloud = self.take_input("cloud")
        if cloud is None or not self._enabled:
            return
        if self._frame_id_override:
            cloud.header.frame_id = self._frame_id_override
        self.publish("cloud", cloud)
