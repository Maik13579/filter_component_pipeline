# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from rclpy.context import Context

from filter_component_base_py.intra_process import get_intra_process_manager
from filter_component_base_py.qos import qos_profile


class Adapter:
    pass


class OtherAdapter:
    pass


class Publisher:
    def __init__(self, count: int) -> None:
        self.count = count

    def get_subscription_count(self) -> int:
        return self.count


def test_context_manager_is_singleton_per_context() -> None:
    context = Context()

    assert get_intra_process_manager(context) is get_intra_process_manager(context)


def test_matches_topic_adapter_and_exact_qos() -> None:
    context = Context()
    manager = get_intra_process_manager(context)
    qos = qos_profile("best_effort", "keep_last", 5, "volatile")
    reliable = qos_profile("reliable", "keep_last", 5, "volatile")
    received: list[object] = []

    manager.register_publisher("pub", "cloud", "/points", Adapter, qos, Publisher(1))
    manager.register_subscription("sub_ok", "cloud", "/points", Adapter, qos, received.append)
    manager.register_subscription("sub_qos", "cloud", "/points", Adapter, reliable, received.append)
    manager.register_subscription("sub_type", "cloud", "/points", OtherAdapter, qos, received.append)
    manager.set_node_active("pub", True)
    manager.set_node_active("sub_ok", True)
    manager.set_node_active("sub_qos", True)
    manager.set_node_active("sub_type", True)

    assert [sub.node_id for sub in manager.compatible_subscriptions("pub", "cloud")] == ["sub_ok"]


def test_publish_deepcopies_for_all_but_last_local_subscriber() -> None:
    context = Context()
    manager = get_intra_process_manager(context)
    qos = qos_profile("best_effort", "keep_last", 5, "volatile")
    received: list[dict[str, int]] = []
    ros_messages: list[dict[str, int]] = []

    manager.register_publisher("pub_multi", "cloud", "/points", Adapter, qos, Publisher(2))
    manager.register_subscription("sub_a", "cloud", "/points", Adapter, qos, received.append)
    manager.register_subscription("sub_b", "cloud", "/points", Adapter, qos, received.append)
    for node_id in ("pub_multi", "sub_a", "sub_b"):
        manager.set_node_active(node_id, True)

    message = {"value": 1}
    manager.publish("pub_multi", "cloud", message, ros_messages.append)

    assert len(received) == 2
    assert received[0] == message
    assert received[0] is not message
    assert received[1] is message
    assert ros_messages == []


def test_ros_echo_suppression_is_recorded_when_ros_publish_is_needed() -> None:
    context = Context()
    manager = get_intra_process_manager(context)
    qos = qos_profile("best_effort", "keep_last", 5, "volatile")
    received: list[dict[str, int]] = []
    ros_messages: list[dict[str, int]] = []

    manager.register_publisher("pub_echo", "cloud", "/points", Adapter, qos, Publisher(2))
    manager.register_subscription("sub_echo", "cloud", "/points", Adapter, qos, received.append)
    manager.set_node_active("pub_echo", True)
    manager.set_node_active("sub_echo", True)

    message = {"value": 1}
    manager.publish("pub_echo", "cloud", message, ros_messages.append)

    assert received == [message]
    assert ros_messages == [message]
    assert manager.should_drop_ros_echo("sub_echo", "cloud")
    assert not manager.should_drop_ros_echo("sub_echo", "cloud")
