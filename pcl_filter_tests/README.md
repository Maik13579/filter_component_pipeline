# pcl_filter_tests

`pcl_filter_tests` contains repository validation coverage for the split package
set. The tests exercise the installed metadata and graph descriptions that tie
the packages together.

Coverage includes:

- filter and logical type discovery through the ament index
- `rclcpp_components` registration metadata for concrete point-type packages
- installed factory example pipeline validation
