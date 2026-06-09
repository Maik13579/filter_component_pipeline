# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
from threading import Lock
from weakref import WeakKeyDictionary
from typing import Any, Callable

from rclpy.context import Context
from rclpy.qos import QoSProfile

from filter_component_base_py.qos import QosKey
from filter_component_base_py.qos import qos_key


@dataclass
class _SubscriptionEndpoint:
    node_id: str
    port_name: str
    topic: str
    adapter_type: type
    qos: QosKey
    callback: Callable[[Any], None]
    active: bool = False


@dataclass
class _PublisherEndpoint:
    node_id: str
    port_name: str
    topic: str
    adapter_type: type
    qos: QosKey
    publisher: Any
    active: bool = False


_managers: WeakKeyDictionary[Context, "IntraProcessManager"] = WeakKeyDictionary()
_managers_lock = Lock()


def get_intra_process_manager(context: Context) -> "IntraProcessManager":
    with _managers_lock:
        manager = _managers.get(context)
        if manager is None:
            manager = IntraProcessManager(context)
            _managers[context] = manager
        return manager


class IntraProcessManager:
    def __init__(self, context: Context) -> None:
        self.context = context
        self._subscriptions: dict[tuple[str, str], _SubscriptionEndpoint] = {}
        self._publishers: dict[tuple[str, str], _PublisherEndpoint] = {}
        self._pending_ros_echoes: dict[tuple[str, str], int] = {}
        self._lock = Lock()

    def register_subscription(
        self,
        node_id: str,
        port_name: str,
        topic: str,
        adapter_type: type,
        qos: QoSProfile,
        callback: Callable[[Any], None],
    ) -> None:
        with self._lock:
            self._subscriptions[(node_id, port_name)] = _SubscriptionEndpoint(
                node_id=node_id,
                port_name=port_name,
                topic=topic,
                adapter_type=adapter_type,
                qos=qos_key(qos),
                callback=callback,
            )

    def register_publisher(
        self,
        node_id: str,
        port_name: str,
        topic: str,
        adapter_type: type,
        qos: QoSProfile,
        publisher: Any,
    ) -> None:
        with self._lock:
            self._publishers[(node_id, port_name)] = _PublisherEndpoint(
                node_id=node_id,
                port_name=port_name,
                topic=topic,
                adapter_type=adapter_type,
                qos=qos_key(qos),
                publisher=publisher,
            )

    def unregister_node(self, node_id: str) -> None:
        with self._lock:
            self._subscriptions = {
                key: endpoint
                for key, endpoint in self._subscriptions.items()
                if endpoint.node_id != node_id
            }
            self._publishers = {
                key: endpoint
                for key, endpoint in self._publishers.items()
                if endpoint.node_id != node_id
            }
            self._pending_ros_echoes = {
                key: count
                for key, count in self._pending_ros_echoes.items()
                if key[0] != node_id
            }

    def set_node_active(self, node_id: str, active: bool) -> None:
        with self._lock:
            for endpoint in self._subscriptions.values():
                if endpoint.node_id == node_id:
                    endpoint.active = active
            for endpoint in self._publishers.values():
                if endpoint.node_id == node_id:
                    endpoint.active = active

    def compatible_subscriptions(self, node_id: str, port_name: str) -> list[_SubscriptionEndpoint]:
        with self._lock:
            publisher = self._publishers.get((node_id, port_name))
            if publisher is None or not publisher.active:
                return []
            return [
                subscription
                for subscription in self._subscriptions.values()
                if subscription.active and self._compatible(publisher, subscription)
            ]

    def publish(
        self,
        node_id: str,
        port_name: str,
        message: Any,
        ros_publish: Callable[[Any], None],
    ) -> None:
        with self._lock:
            publisher = self._publishers.get((node_id, port_name))
            if publisher is None:
                raise KeyError(f"Unknown publisher port '{node_id}:{port_name}'")
            subscriptions = [
                subscription
                for subscription in self._subscriptions.values()
                if subscription.active and self._compatible(publisher, subscription)
            ]
            ros_subscription_count = publisher.publisher.get_subscription_count()
        for subscription in subscriptions[:-1]:
            subscription.callback(deepcopy(message))
        if subscriptions:
            subscriptions[-1].callback(message)
        publish_to_ros = ros_subscription_count > len(subscriptions)
        if publish_to_ros:
            with self._lock:
                for subscription in subscriptions:
                    key = (subscription.node_id, subscription.port_name)
                    self._pending_ros_echoes[key] = self._pending_ros_echoes.get(key, 0) + 1
            ros_publish(message)

    def should_drop_ros_echo(self, node_id: str, port_name: str) -> bool:
        with self._lock:
            key = (node_id, port_name)
            count = self._pending_ros_echoes.get(key, 0)
            if count <= 0:
                return False
            if count == 1:
                self._pending_ros_echoes.pop(key, None)
            else:
                self._pending_ros_echoes[key] = count - 1
            return True

    @staticmethod
    def _compatible(
        publisher: _PublisherEndpoint,
        subscription: _SubscriptionEndpoint,
    ) -> bool:
        return (
            publisher.topic == subscription.topic
            and publisher.adapter_type is subscription.adapter_type
            and publisher.qos == subscription.qos
        )
