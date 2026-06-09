# filter_component_factory_py

Python runtime factory for filter component pipelines.

This package loads the Python side of a mixed filter pipeline. It uses the same
YAML graph as the C++ `filter_component_factory`. A filter node is treated as
Python when it has `python_module` and `python_class`; C++ nodes continue to use
`component_class`.

## YAML Node Format

```yaml
- type: filter
  name: MyPythonFilter_1
  python_module: my_filter_package.filters
  python_class: MyPythonFilter
  input_ports: cloud:PointCloud2
  output_ports: cloud:PointCloud2
  parameters:
    filter.example: 1.0
```

Edges and per-port QoS use the same graph format as C++ filter components.

## Component Container

The Python runtime starts a small component-container equivalent. It exposes the
same `composition_interfaces` service shape as the C++ component container:
`_container/load_node`, `_container/unload_node`, and `_container/list_nodes`.

This intentionally mimics the C++ component container interface so the rqt
editor can load and unload Python filters with the same live-runtime logic it
uses for C++ filters. The `LoadNode.plugin_name` field carries the Python plugin
identity as `<module>:<class>`, for example
`my_filter_package.filters:MyPythonFilter`.

## Running

```bash
ros2 run filter_component_factory_py filter_pipeline_factory_py
```

The editor starts this process automatically for live Python filters. It appears
as `/filter_component_editor_python_factory` when launched by the editor.
Python-to-Python edges can use the Python intra-process manager; C++-to-Python
and Python-to-C++ edges currently use normal ROS topics.

## Python Filter Registration

Python filters can be advertised to discovery tools with a `package.xml` export:

```xml
<export>
  <filter_component_py
    filter="MyPythonFilter"
    module="my_filter_package.filters"
    class="MyPythonFilter"
    input="PointCloud2"
    output="PointCloud2"
    input_ports="cloud:PointCloud2"
    output_ports="cloud:PointCloud2"/>
</export>
```
