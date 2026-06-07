// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__PASSTHROUGH_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__PASSTHROUGH_FILTER_HPP_

#include <memory>
#include <string>
#include <vector>

#include <pcl/filters/passthrough.h>
#include <pcl/point_cloud.h>

namespace pcl_filter_components::filters
{

template <typename PointT>
class PassThroughFilter
{
public:
  using PointType = PointT;
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;

  struct Params
  {
    std::string field_name;
    double min_value;
    double max_value;
    bool invert;
  };

  void configure(const Params & params)
  {
    params_ = params;
  }

  void filter(const Cloud & input, Cloud & output) const
  {
    pcl::PassThrough<PointT> passthrough;
    passthrough.setFilterFieldName(params_.field_name);
    passthrough.setFilterLimits(params_.min_value, params_.max_value);
    passthrough.setNegative(params_.invert);
    passthrough.setInputCloud(makeConstPtr(input));
    passthrough.filter(output);
  }

  void filterIndices(const Cloud & input, Indices & output) const
  {
    pcl::PassThrough<PointT> passthrough;
    passthrough.setFilterFieldName(params_.field_name);
    passthrough.setFilterLimits(params_.min_value, params_.max_value);
    passthrough.setNegative(params_.invert);
    passthrough.setInputCloud(makeConstPtr(input));
    passthrough.filter(output);
  }

private:
  static typename Cloud::ConstPtr makeConstPtr(const Cloud & cloud)
  {
    return typename Cloud::ConstPtr(&cloud, [](const Cloud *) {});
  }

  Params params_{"z", -1.0, 2.0, false};
};

}  // namespace pcl_filter_components::filters

#endif  // PCL_FILTER_COMPONENTS__FILTERS__PASSTHROUGH_FILTER_HPP_
