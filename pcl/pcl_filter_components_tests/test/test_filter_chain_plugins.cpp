// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include <filters/filter_base.hpp>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pluginlib/class_list_macros.hpp>

namespace pcl_filter_components_tests
{

class IntensityOffsetXYZIFilter
  : public filters::FilterBase<pcl::PointCloud<pcl::PointXYZI>>
{
public:
  bool update(
    const pcl::PointCloud<pcl::PointXYZI> & data_in,
    pcl::PointCloud<pcl::PointXYZI> & data_out) override
  {
    data_out = data_in;
    for (auto & point : data_out.points) {
      point.intensity += offset_;
    }
    return true;
  }

protected:
  bool configure() override
  {
    return getParam("offset", offset_, false, 1.0);
  }

private:
  double offset_{1.0};
};

class FrameIdSuffixXYZIFilter
  : public filters::FilterBase<pcl::PointCloud<pcl::PointXYZI>>
{
public:
  bool update(
    const pcl::PointCloud<pcl::PointXYZI> & data_in,
    pcl::PointCloud<pcl::PointXYZI> & data_out) override
  {
    data_out = data_in;
    data_out.header.frame_id += suffix_;
    return true;
  }

protected:
  bool configure() override
  {
    return getParam("suffix", suffix_, false, std::string{"_filtered"});
  }

private:
  std::string suffix_{"_filtered"};
};

}  // namespace pcl_filter_components_tests

PLUGINLIB_EXPORT_CLASS(
  pcl_filter_components_tests::IntensityOffsetXYZIFilter,
  filters::FilterBase<pcl::PointCloud<pcl::PointXYZI>>)

PLUGINLIB_EXPORT_CLASS(
  pcl_filter_components_tests::FrameIdSuffixXYZIFilter,
  filters::FilterBase<pcl::PointCloud<pcl::PointXYZI>>)
