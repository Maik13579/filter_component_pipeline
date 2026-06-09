# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass

from rclpy.qos import DurabilityPolicy
from rclpy.qos import HistoryPolicy
from rclpy.qos import QoSProfile
from rclpy.qos import ReliabilityPolicy


@dataclass(frozen=True)
class QosKey:
    reliability: str
    history: str
    depth: int
    durability: str


def qos_key(qos: QoSProfile) -> QosKey:
    return QosKey(
        reliability=_reliability_name(qos.reliability),
        history=_history_name(qos.history),
        depth=int(qos.depth),
        durability=_durability_name(qos.durability),
    )


def qos_profile(reliability: str, history: str, depth: int, durability: str) -> QoSProfile:
    qos = QoSProfile(depth=max(1, int(depth)))
    if reliability == "best_effort":
        qos.reliability = ReliabilityPolicy.BEST_EFFORT
    elif reliability == "reliable":
        qos.reliability = ReliabilityPolicy.RELIABLE
    else:
        raise ValueError(f"Unsupported QoS reliability '{reliability}'")
    if history == "keep_last":
        qos.history = HistoryPolicy.KEEP_LAST
    elif history == "keep_all":
        qos.history = HistoryPolicy.KEEP_ALL
    else:
        raise ValueError(f"Unsupported QoS history '{history}'")
    if durability == "volatile":
        qos.durability = DurabilityPolicy.VOLATILE
    elif durability == "transient_local":
        qos.durability = DurabilityPolicy.TRANSIENT_LOCAL
    else:
        raise ValueError(f"Unsupported QoS durability '{durability}'")
    return qos


def _reliability_name(value: ReliabilityPolicy) -> str:
    if value == ReliabilityPolicy.BEST_EFFORT:
        return "best_effort"
    if value == ReliabilityPolicy.RELIABLE:
        return "reliable"
    return str(value)


def _history_name(value: HistoryPolicy) -> str:
    if value == HistoryPolicy.KEEP_LAST:
        return "keep_last"
    if value == HistoryPolicy.KEEP_ALL:
        return "keep_all"
    return str(value)


def _durability_name(value: DurabilityPolicy) -> str:
    if value == DurabilityPolicy.VOLATILE:
        return "volatile"
    if value == DurabilityPolicy.TRANSIENT_LOCAL:
        return "transient_local"
    return str(value)
