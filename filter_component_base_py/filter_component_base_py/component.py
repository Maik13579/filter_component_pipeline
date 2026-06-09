# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from typing import Any

from rclpy.lifecycle import LifecycleNode
from rclpy.lifecycle import TransitionCallbackReturn
from rclpy.parameter import Parameter

from filter_component_base_py.intra_process import get_intra_process_manager
from filter_component_base_py.ports import PortDescriptor
from filter_component_base_py.qos import qos_profile
from filter_component_synchronizer_py import FilterComponentSynchronizer
from filter_component_synchronizer_py import SynchronizerOptions


class FilterComponentBasePy(LifecycleNode):
    def __init__(
        self,
        node_name: str,
        input_ports: list[PortDescriptor],
        output_ports: list[PortDescriptor],
        **kwargs: Any,
    ) -> None:
        super().__init__(node_name, **kwargs)
        self.input_ports = list(input_ports)
        self.output_ports = list(output_ports)
        self._node_id = node_name
        self._manager = get_intra_process_manager(self.context)
        self._synchronizer: FilterComponentSynchronizer | None = None
        self._port_publishers: dict[str, Any] = {}
        self._port_subscriptions: dict[str, Any] = {}
        self._active = False
        for port in self.input_ports:
            self._declare_port_parameters("inputs", port)
        for port in self.output_ports:
            self._declare_port_parameters("outputs", port)
        if len(self.input_ports) > 1:
            self.declare_parameter("sync.mode", "receipt_time")
            self.declare_parameter("sync.queue_size", 10)
            self.declare_parameter("sync.max_interval", 0.05)

    def configure_filter(self) -> None:
        pass

    def process(self) -> None:
        raise NotImplementedError

    def on_configure(self, state: State) -> TransitionCallbackReturn:
        del state
        try:
            self.configure_filter()
            self._configure_interfaces()
            return TransitionCallbackReturn.SUCCESS
        except Exception as exc:
            self.get_logger().error(f"Failed to configure filter component: {exc}")
            self._cleanup_interfaces()
            return TransitionCallbackReturn.FAILURE

    def on_activate(self, state: State) -> TransitionCallbackReturn:
        del state
        self._active = True
        self._manager.set_node_active(self._node_id, True)
        return TransitionCallbackReturn.SUCCESS

    def on_deactivate(self, state: State) -> TransitionCallbackReturn:
        del state
        self._active = False
        self._manager.set_node_active(self._node_id, False)
        return TransitionCallbackReturn.SUCCESS

    def on_cleanup(self, state: State) -> TransitionCallbackReturn:
        del state
        self._cleanup_interfaces()
        self._active = False
        return TransitionCallbackReturn.SUCCESS

    def on_shutdown(self, state: State) -> TransitionCallbackReturn:
        del state
        self._cleanup_interfaces()
        self._active = False
        return TransitionCallbackReturn.SUCCESS

    def take_input(self, port_name: str) -> Any | None:
        if self._synchronizer is None:
            raise RuntimeError("Cannot take input before interfaces are configured")
        port = self._input_port(port_name)
        return self._synchronizer.take_input(port_name, port.adapter_type)

    def peek_input(self, port_name: str) -> Any | None:
        if self._synchronizer is None:
            raise RuntimeError("Cannot peek input before interfaces are configured")
        port = self._input_port(port_name)
        return self._synchronizer.peek_input(port_name, port.adapter_type)

    def publish(self, port_name: str, message: Any) -> None:
        port = self._output_port(port_name)

        def ros_publish(custom_message: Any) -> None:
            self._port_publishers[port_name].publish(port.adapter_type.to_ros(custom_message))

        self._manager.publish(self._node_id, port_name, message, ros_publish)

    def _configure_interfaces(self) -> None:
        options = SynchronizerOptions()
        if len(self.input_ports) > 1:
            options = SynchronizerOptions(
                mode=self.get_parameter("sync.mode").value,
                queue_size=self.get_parameter("sync.queue_size").value,
                max_interval=self.get_parameter("sync.max_interval").value,
            )
        self._synchronizer = FilterComponentSynchronizer(options, self._process_synchronized_inputs)
        for port in self.input_ports:
            topic = self.get_parameter(self._topic_parameter_name("inputs", port.name)).value
            qos = self._port_qos("inputs", port)
            self._synchronizer.add_input(port.name, port.adapter_type)
            subscription = self.create_subscription(
                port.adapter_type.ros_type,
                topic,
                lambda msg, info=None, port=port: self._store_ros_message(port, msg),
                qos,
            )
            self._port_subscriptions[port.name] = subscription
            self._manager.register_subscription(
                self._node_id,
                port.name,
                topic,
                port.adapter_type,
                qos,
                lambda msg, port=port: self._store_custom_message(port, msg),
            )
        for port in self.output_ports:
            topic = self.get_parameter(self._topic_parameter_name("outputs", port.name)).value
            qos = self._port_qos("outputs", port)
            publisher = self.create_publisher(port.adapter_type.ros_type, topic, qos)
            self._port_publishers[port.name] = publisher
            self._manager.register_publisher(
                self._node_id,
                port.name,
                topic,
                port.adapter_type,
                qos,
                publisher,
            )

    def _cleanup_interfaces(self) -> None:
        for publisher in list(self._port_publishers.values()):
            try:
                self.destroy_publisher(publisher)
            except Exception:
                pass
        for subscription in list(self._port_subscriptions.values()):
            try:
                self.destroy_subscription(subscription)
            except Exception:
                pass
        self._manager.unregister_node(self._node_id)
        self._synchronizer = None
        self._port_publishers.clear()
        self._port_subscriptions.clear()

    def _store_ros_message(self, port: PortDescriptor, message: Any) -> None:
        if self._manager.should_drop_ros_echo(self._node_id, port.name):
            return
        self._store_custom_message(port, port.adapter_type.from_ros(message))

    def _store_custom_message(self, port: PortDescriptor, message: Any) -> None:
        if self._synchronizer is None:
            return
        self._synchronizer.store_message(
            port.name,
            port.adapter_type,
            message,
            self.get_clock().now().nanoseconds,
        )

    def _process_synchronized_inputs(self) -> None:
        if not self._active or self._synchronizer is None:
            return
        self.process()

    def _declare_port_parameters(self, direction: str, port: PortDescriptor) -> None:
        self.declare_parameter(
            self._topic_parameter_name(direction, port.name),
            self._default_port_topic(direction, port.name),
        )
        self.declare_parameter(self._qos_parameter_name(direction, port.name, "reliability"), port.default_reliability)
        self.declare_parameter(self._qos_parameter_name(direction, port.name, "history"), port.default_history)
        self.declare_parameter(self._qos_parameter_name(direction, port.name, "depth"), port.default_depth)
        self.declare_parameter(self._qos_parameter_name(direction, port.name, "durability"), port.default_durability)

    def _port_qos(self, direction: str, port: PortDescriptor):
        return qos_profile(
            self.get_parameter(self._qos_parameter_name(direction, port.name, "reliability")).value,
            self.get_parameter(self._qos_parameter_name(direction, port.name, "history")).value,
            self.get_parameter(self._qos_parameter_name(direction, port.name, "depth")).value,
            self.get_parameter(self._qos_parameter_name(direction, port.name, "durability")).value,
        )

    def _input_port(self, port_name: str) -> PortDescriptor:
        return self._find_port(self.input_ports, port_name, "input")

    def _output_port(self, port_name: str) -> PortDescriptor:
        return self._find_port(self.output_ports, port_name, "output")

    @staticmethod
    def _find_port(ports: list[PortDescriptor], port_name: str, direction: str) -> PortDescriptor:
        for port in ports:
            if port.name == port_name:
                return port
        raise KeyError(f"Unknown {direction} port '{port_name}'")

    @staticmethod
    def _topic_parameter_name(direction: str, port_name: str) -> str:
        return f"{direction}.{port_name}.topic"

    @staticmethod
    def _qos_parameter_name(direction: str, port_name: str, field: str) -> str:
        return f"{direction}.{port_name}.qos.{field}"

    @staticmethod
    def _default_port_topic(direction: str, port_name: str) -> str:
        singular = "input" if direction == "inputs" else "output"
        return f"~/{singular}/{port_name}"
