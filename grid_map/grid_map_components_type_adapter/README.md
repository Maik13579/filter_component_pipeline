# grid_map_components_type_adapter

`grid_map_components_type_adapter` provides a header-only `rclcpp::TypeAdapter`
for `grid_map::GridMap`.

The logical filter component type is `GridMap`, and the ROS message
representation is `grid_map_msgs/msg/GridMap`. Conversion uses the upstream
`grid_map::GridMapRosConverter` APIs from `grid_map_ros`.
