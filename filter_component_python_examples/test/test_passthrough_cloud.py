# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import inspect

from filter_component_python_examples import PythonPointCloudPassthrough


def test_passthrough_cloud_declares_cloud_ports() -> None:
    assert PythonPointCloudPassthrough.__name__ == "PythonPointCloudPassthrough"


def test_passthrough_cloud_reads_parameters_during_configure() -> None:
    configure_source = inspect.getsource(PythonPointCloudPassthrough.configure_filter)
    process_source = inspect.getsource(PythonPointCloudPassthrough.process)

    assert "get_parameter" in configure_source
    assert "get_parameter" not in process_source
