# pcl_filter_components_tests

`pcl_filter_components_tests` contains repository validation coverage for the split package
set. The tests exercise the installed metadata and graph descriptions that tie
the packages together.

Coverage includes:

- filter and logical type discovery through the ament index
- `rclcpp_components` registration metadata for concrete point-type packages
- installed factory example pipeline validation
- parameter descriptor coverage for port topic and QoS metadata
