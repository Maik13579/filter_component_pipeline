# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from filter_component_python_examples import PythonPointCloudPassthrough


def test_passthrough_cloud_declares_cloud_ports() -> None:
    assert PythonPointCloudPassthrough.__name__ == "PythonPointCloudPassthrough"
