# pcl_filter_type_adapters

`pcl_filter_type_adapters` provides header-only `rclcpp::TypeAdapter`
definitions for stamped PCL clouds and PCL point indices. These adapters are the
bridge between logical pipeline types and ROS message types.

Cloud logical types such as `PointXYZ`, `PointXYZI`, `PointXYZRGB`,
`PointXYZRGBA`, and `PointNormal` use `sensor_msgs/msg/PointCloud2` as their ROS
message representation. The `PointIndices` logical type uses
`pcl_msgs/msg/PointIndices`.

This package exports editor discovery metadata for shared non-filter types:

- `PointNormal`
- `PointIndices`

Concrete point-type packages export their own cloud aliases and filter metadata.
Packages that need typed PCL subscriptions or publishers include headers from
`pcl_filter_type_adapters/ros/...` and select the adapter that matches the
logical port type.
