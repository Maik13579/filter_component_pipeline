# filter_component_base_py

Python lifecycle base classes for filter component pipelines.

This package provides the Python equivalent of the C++ `filter_component_base`
framework. It is intended for fast prototyping while keeping the same port,
topic, QoS, lifecycle, and graph concepts as the C++ components.

## Features

- `FilterComponentBasePy`, based on `rclpy.lifecycle.LifecycleNode`.
- Input and output `PortDescriptor` helpers.
- Python parameter helpers:
  `declare_parameter_if_not_declared()`, `get_parameter()`, and descriptor
  builders for plain, integer-range, and floating-point-range parameters.
- Per-port topic parameters:
  - `inputs.<port>.topic`
  - `outputs.<port>.topic`
- Per-port QoS parameters:
  - `inputs.<port>.qos.reliability`
  - `inputs.<port>.qos.history`
  - `inputs.<port>.qos.depth`
  - `inputs.<port>.qos.durability`
  - and matching `outputs.<port>.qos.*` parameters.
- Multi-input synchronization through `filter_component_synchronizer_py`.
- Context-scoped Python intra-process manager for Python-to-Python direct
  delivery.
- `PointCloud2NumpyAdapter`, built on `sensor_msgs_py.point_cloud2`.

## Example Filter

```python
from filter_component_base_py import FilterComponentBasePy
from filter_component_base_py import input_port
from filter_component_base_py import output_port
from filter_component_base_py.adapters import PointCloud2NumpyAdapter


class PassthroughCloud(FilterComponentBasePy):
    def __init__(self, node_name: str = "passthrough_cloud"):
        super().__init__(
            node_name,
            input_ports=[input_port("cloud", PointCloud2NumpyAdapter)],
            output_ports=[output_port("cloud", PointCloud2NumpyAdapter)],
        )

    def configure_filter(self) -> None:
        pass

    def process(self) -> None:
        cloud = self.take_input("cloud")
        if cloud is not None:
            self.publish("cloud", cloud)
```

## Python Type Adapters

Python adapters are small classes that mimic the role of C++ `rclcpp` type
adapters. They define the ROS message type used on the wire and the custom
Python type used inside filters:

```python
class ExampleAdapter:
    ros_type = ExampleRosMessage
    custom_type = ExamplePythonObject

    @staticmethod
    def from_ros(message: ExampleRosMessage) -> ExamplePythonObject:
        ...

    @staticmethod
    def to_ros(message: ExamplePythonObject) -> ExampleRosMessage:
        ...
```

The Python base class uses the adapter's `ros_type` to create normal ROS
publishers and subscriptions. Incoming ROS messages are converted with
`from_ros()` before they enter the synchronizer. Published custom objects are
converted with `to_ros()` only when the message has to leave the Python
intra-process path and go through ROS.

This package includes `PointCloud2NumpyAdapter`. It wraps `sensor_msgs_py.point_cloud2`
and exposes `sensor_msgs/msg/PointCloud2` as a structured NumPy array plus
metadata. This keeps Python filters close to the data representation they
usually want while preserving normal ROS topic compatibility.

The package exports the logical type `PointCloud2` in `package.xml`, mapped to
`sensor_msgs/msg/PointCloud2` and
`filter_component_base_py.adapters.PointCloud2NumpyAdapter`. This makes the type
available in the rqt editor's type filters.

## Intra-process Behavior

The Python intra-process manager is a context-scoped singleton. Each
`FilterComponentBasePy` gets the manager for its `rclpy.Context` during
construction. During lifecycle configuration, the filter registers each input
and output port with:

- the owning node id,
- the port name,
- the resolved topic,
- the adapter class,
- and the full QoS profile.

The manager then mimics the intent of the C++ `rclcpp` intra-process path for
Python filters. It searches for direct Python-to-Python connections with the
same topic, same adapter type, and exact same QoS settings. The exact QoS check
uses reliability, history, depth, and durability.

Every Python filter still creates normal ROS publishers and subscriptions. The
direct path is an optimization layered on top of the ROS graph, not a replacement
for it. This keeps the topic visible to external ROS nodes and allows non-Python
publishers/subscribers to participate.

On publish:

1. The manager finds active compatible Python subscribers.
2. Compatible Python subscribers receive the adapter custom object directly.
3. If multiple compatible Python subscribers exist, every subscriber except the
   last receives a deep copy; the last subscriber receives the original object.
4. If the ROS publisher has additional subscribers beyond the matched
   intra-process subscribers, the message is converted back to the ROS message
   type and published through the ROS publisher.
5. When both direct delivery and ROS publishing happen, the manager records one
   pending ROS echo for each direct-delivered Python subscriber. The subscriber's
   ROS callback consumes that pending echo and drops the ROS copy, so the filter
   does not process the same local publication twice.

This is intentionally close to the C++ behavior goal: avoid serialization and
copies inside one process when the endpoints are known to be compatible, while
preserving normal ROS topic behavior whenever other nodes are present.

The C++ `rclcpp` implementation can filter duplicate ROS delivery by comparing
publisher GIDs in message metadata. Jazzy `rclpy` does not expose the publisher
GID through the public subscription callback metadata, so this Python package
uses pending-echo suppression instead. If rclpy exposes publisher GIDs in a
future ROS 2 release, the manager can switch to the same GID-based check as
`rclcpp`.
