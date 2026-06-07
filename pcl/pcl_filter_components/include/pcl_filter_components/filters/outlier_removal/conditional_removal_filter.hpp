// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__CONDITIONAL_REMOVAL_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__CONDITIONAL_REMOVAL_FILTER_HPP_

#include <memory>
#include <string>
#include <vector>
#include <pcl/filters/conditional_removal.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::outlier_removal
{
template <typename PointT>
class ConditionalRemovalFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    std::string field_name;
    double min_value;
    double max_value;
    bool invert;
  };
  static const char * nodeName()
  {
    return "conditional_removal_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    if (params_.invert)
    {
      common::copyIf(input, output, [this](const auto & p) {
          return accept(p);
        }
                     );
      return;
    }
    auto f = make();
    f.setInputCloud(common::makeConstPtr(input));
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    common::indicesIf(input, output, [this](const auto & p) {
        return accept(p);
      }
                      );
  }
private:
  bool accept(const PointT & p) const
  {
    double value = p.z;
    if (params_.field_name == "x")
    {
      value = p.x;
    }
    else if (params_.field_name == "y")
    {
      value = p.y;
    }
    const bool inside = value >= params_.min_value && value <= params_.max_value;
    return params_.invert ? !inside : inside;
  }
  pcl::ConditionalRemoval<PointT> make() const
  {
    using Comparison = pcl::FieldComparison<PointT>;
    typename pcl::ConditionAnd<PointT>::Ptr condition(new pcl::ConditionAnd<PointT>());
    condition->addComparison(typename Comparison::ConstPtr(new Comparison(params_.field_name, pcl::ComparisonOps::GE, params_.min_value)));
    condition->addComparison(typename Comparison::ConstPtr(new Comparison(params_.field_name, pcl::ComparisonOps::LE, params_.max_value)));
    pcl::ConditionalRemoval<PointT> f;
    f.setCondition(condition);
    return f;
  }
  Params params_{"z", -1.0, 2.0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__OUTLIER_REMOVAL__CONDITIONAL_REMOVAL_FILTER_HPP_
