# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from sensor_msgs.msg import PointCloud2
from sensor_msgs_py import point_cloud2
from std_msgs.msg import Header


@dataclass
class PointCloud2Array:
    points: np.ndarray
    header: Header
    fields: list
    height: int = 1
    width: int = 0
    is_bigendian: bool = False
    point_step: int | None = None
    is_dense: bool = True


class PointCloud2NumpyAdapter:
    ros_type = PointCloud2
    custom_type = PointCloud2Array

    @staticmethod
    def from_ros(message: PointCloud2) -> PointCloud2Array:
        points = point_cloud2.read_points(message, reshape_organized_cloud=False)
        return PointCloud2Array(
            points=points,
            header=message.header,
            fields=list(message.fields),
            height=message.height,
            width=message.width,
            is_bigendian=message.is_bigendian,
            point_step=message.point_step,
            is_dense=message.is_dense,
        )

    @staticmethod
    def to_ros(message: PointCloud2Array | PointCloud2 | np.ndarray) -> PointCloud2:
        if isinstance(message, PointCloud2):
            return message
        if isinstance(message, np.ndarray):
            if message.dtype.names is None:
                raise TypeError("PointCloud2 numpy arrays must use a structured dtype")
            fields = _fields_from_dtype(message.dtype)
            return point_cloud2.create_cloud(Header(), fields, message)
        fields = message.fields or _fields_from_dtype(message.points.dtype)
        cloud = point_cloud2.create_cloud(message.header, fields, message.points, message.point_step)
        cloud.height = message.height
        cloud.width = message.width or len(message.points)
        cloud.is_bigendian = message.is_bigendian
        cloud.is_dense = message.is_dense
        return cloud


def _fields_from_dtype(dtype: np.dtype) -> list:
    if dtype.names is None:
        raise TypeError("PointCloud2 dtype must be structured")
    dummy = PointCloud2()
    dummy.fields = []
    dummy.point_step = dtype.itemsize
    return point_cloud2.dtype_from_fields.__globals__["dtype_to_fields"](dtype) \
        if "dtype_to_fields" in point_cloud2.dtype_from_fields.__globals__ else _fallback_fields(dtype)


def _fallback_fields(dtype: np.dtype) -> list:
    from sensor_msgs.msg import PointField

    datatype_by_dtype = {
        np.dtype("int8"): PointField.INT8,
        np.dtype("uint8"): PointField.UINT8,
        np.dtype("int16"): PointField.INT16,
        np.dtype("uint16"): PointField.UINT16,
        np.dtype("int32"): PointField.INT32,
        np.dtype("uint32"): PointField.UINT32,
        np.dtype("float32"): PointField.FLOAT32,
        np.dtype("float64"): PointField.FLOAT64,
    }
    fields = []
    for name in dtype.names or ():
        field_dtype, offset = dtype.fields[name][:2]
        base_dtype = field_dtype.base
        count = int(np.prod(field_dtype.shape)) if field_dtype.shape else 1
        fields.append(
            PointField(
                name=name,
                offset=offset,
                datatype=datatype_by_dtype[base_dtype],
                count=count,
            )
        )
    return fields
