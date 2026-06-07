// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__STATISTICAL_OUTLIER_REMOVAL_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__STATISTICAL_OUTLIER_REMOVAL_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::outlier_removal
{
template <typename PointT>
class StatisticalOutlierRemovalFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    int mean_k;
    double stddev;
    bool invert;
  };
  static const char * nodeName()
  {
    return "statistical_outlier_removal_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
private:
  pcl::StatisticalOutlierRemoval<PointT> make() const
  {
    pcl::StatisticalOutlierRemoval<PointT> f;
    f.setMeanK(std::max(params_.mean_k, 1));
    f.setStddevMulThresh(params_.stddev);
    f.setNegative(params_.invert);
    return f;
  }
  Params params_{20, 1.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__STATISTICAL_OUTLIER_REMOVAL_FILTER_HPP_
