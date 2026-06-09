# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from filter_component_editor.runtime import LivePipelineRuntime, LivePythonPipelineRuntime, LoadedComponent


class RecordingRuntime(LivePipelineRuntime):
    def __init__(self) -> None:
        super().__init__()
        self.calls: list[tuple[str, str]] = []

    def start(self) -> None:
        pass

    def _reconcile_loaded_components(self) -> None:
        pass

    def load(self, node_id: str, spec: dict[str, object], configure: bool = True) -> None:
        self.calls.append(("load", node_id))
        self.loaded[node_id] = LoadedComponent(
            unique_id=len(self.loaded) + 1,
            full_node_name=f"/{node_id}",
            package=str(spec["package"]),
            component_class=str(spec["component_class"]),
            parameters=dict(spec["parameters"]),
            configured=configure,
        )

    def unload(self, node_id: str, ensure_started: bool = True) -> None:
        self.calls.append(("unload", node_id))
        self.loaded.pop(node_id, None)

    def reconfigure(self, node_id: str, parameters: dict[str, object]) -> None:
        self.calls.append(("reconfigure", node_id))
        self.loaded[node_id].parameters = dict(parameters)

class RecordingPythonRuntime(LivePythonPipelineRuntime):
    def __init__(self) -> None:
        super().__init__()
        self.calls: list[tuple[str, str]] = []

    def start(self) -> None:
        pass

    def _reconcile_loaded_components(self) -> None:
        pass

    def load(self, node_id: str, spec: dict[str, object], configure: bool = True) -> None:
        self.calls.append(("load", node_id))
        self.loaded[node_id] = LoadedComponent(
            unique_id=len(self.loaded) + 1,
            full_node_name=f"/{node_id}",
            package=str(spec["package"]),
            component_class=str(spec["component_class"]),
            parameters=dict(spec["parameters"]),
            configured=configure,
        )

    def unload(self, node_id: str, ensure_started: bool = True) -> None:
        self.calls.append(("unload", node_id))
        self.loaded.pop(node_id, None)

    def reconfigure(self, node_id: str, parameters: dict[str, object]) -> None:
        self.calls.append(("reconfigure", node_id))
        self.loaded[node_id].parameters = dict(parameters)


def chain_spec(parameters: dict[str, object]) -> dict[str, object]:
    return {
        "package": "pkg",
        "component_class": "pkg::ChainComponent",
        "parameters": parameters,
        "reload_on_parameter_change": True,
    }


def test_reload_on_parameter_change_skips_reconfigure_when_unchanged() -> None:
    runtime = RecordingRuntime()
    parameters = {
        "filters.filter1.name": "first",
        "filters.filter1.type": "pkg/First",
    }
    runtime.loaded["chain"] = LoadedComponent(
        unique_id=1,
        full_node_name="/chain",
        package="pkg",
        component_class="pkg::ChainComponent",
        parameters=dict(parameters),
        configured=True,
    )

    runtime.sync({"chain": chain_spec(dict(parameters))})

    assert runtime.calls == []


def test_reload_on_parameter_change_reloads_when_changed() -> None:
    runtime = RecordingRuntime()
    runtime.loaded["chain"] = LoadedComponent(
        unique_id=1,
        full_node_name="/chain",
        package="pkg",
        component_class="pkg::ChainComponent",
        parameters={
            "filters.filter1.name": "first",
            "filters.filter1.type": "pkg/First",
        },
        configured=True,
    )

    runtime.sync({
        "chain": chain_spec({
            "filters.filter1.name": "first",
            "filters.filter1.type": "pkg/First",
            "filters.filter1.params.leaf_size": 0.2,
        })
    })

    assert runtime.calls == [("unload", "chain"), ("load", "chain")]


def test_python_runtime_loads_and_reloads_components() -> None:
    runtime = RecordingPythonRuntime()
    spec = {
        "package": "py_pkg",
        "component_class": "py_pkg.filters:PyFilter",
        "parameters": {"outputs.cloud.topic": "/cloud"},
    }

    runtime.sync({"PyFilter": spec})
    runtime.sync({"PyFilter": spec})
    runtime.sync({
        "PyFilter": {
            "package": "py_pkg",
            "component_class": "py_pkg.filters:PyFilter",
            "parameters": {"outputs.cloud.topic": "/filtered"},
        }
    })

    assert runtime.calls == [
        ("load", "PyFilter"),
        ("reconfigure", "PyFilter"),
        ("unload", "PyFilter"),
        ("load", "PyFilter"),
    ]


def test_cpp_runtime_does_not_start_for_empty_graph() -> None:
    runtime = RecordingRuntime()

    runtime.sync({})

    assert runtime.calls == []
