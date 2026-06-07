// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SELECTION__EXTRACT_INDICES_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SELECTION__EXTRACT_INDICES_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/filters/extract_indices.h>
#include <pcl/point_cloud.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::selection
{
template <typename PointT>
class ExtractIndicesFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    int start_index;
    int count;
    bool invert;
    bool keep_organized;
  };
  static const char * nodeName()
  {
    return "extract_indices_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    Indices idx;
    selected(input, idx);
    pcl::PointIndices::Ptr pi(new pcl::PointIndices);
    pi->indices = idx;
    pcl::ExtractIndices<PointT> f;
    f.setInputCloud(common::makeConstPtr(input));
    f.setIndices(pi);
    f.setNegative(params_.invert);
    f.setKeepOrganized(params_.keep_organized);
    f.filter(output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    selected(input, output);
    if (params_.invert)
    {
      std::vector<bool> s(input.size(), false);
      for (int i: output)
      {
        s[static_cast<std::size_t>(i)] = true;
      }
      output.clear();
      for (std::size_t i = 0; i < input.size(); ++i)
      {
        if (!s[i])
        {
          output.push_back(static_cast<int>(i));
        }
      }
    }
  }
private:
  void selected(const Cloud & input, Indices & output) const
  {
    output.clear();
    const int first = std::max(params_.start_index, 0);
    const int last = std::min(first + std::max(params_.count, 0), static_cast<int>(input.size()));
    for (int i = first; i < last; ++i)
    {
      output.push_back(i);
    }
  }
  Params params_{0, 1000000, false, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SELECTION__EXTRACT_INDICES_FILTER_HPP_
