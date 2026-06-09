# Copyright 2026 Maik Knof
# SPDX-License-Identifier: Apache-2.0

import numpy as np
from sensor_msgs.msg import PointField
from sensor_msgs_py import point_cloud2
from std_msgs.msg import Header

from filter_component_base_py.adapters import PointCloud2NumpyAdapter


def test_point_cloud2_adapter_wraps_sensor_msgs_py_structured_array() -> None:
    fields = [
        PointField(name="x", offset=0, datatype=PointField.FLOAT32, count=1),
        PointField(name="y", offset=4, datatype=PointField.FLOAT32, count=1),
        PointField(name="z", offset=8, datatype=PointField.FLOAT32, count=1),
    ]
    cloud = point_cloud2.create_cloud(Header(frame_id="map"), fields, [(1.0, 2.0, 3.0)])

    adapted = PointCloud2NumpyAdapter.from_ros(cloud)

    assert isinstance(adapted.points, np.ndarray)
    assert adapted.points.dtype.names == ("x", "y", "z")
    assert adapted.points["x"][0] == 1.0
    assert adapted.header.frame_id == "map"
