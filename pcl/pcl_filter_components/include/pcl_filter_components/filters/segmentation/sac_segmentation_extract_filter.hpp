// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__SAC_SEGMENTATION_EXTRACT_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__SAC_SEGMENTATION_EXTRACT_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/common/io.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/point_cloud.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::segmentation
{
template <typename PointT>
class SACSegmentationExtractFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double distance_threshold;
    int max_iterations;
    bool optimize_coefficients;
    bool invert;
  };
  static const char * nodeName()
  {
    return "sac_segmentation_extract_filter";
  }
  void configure(const Params & p)
  {
    params_ = p;
  }
  void filter(const Cloud & input, Cloud & output) const
  {
    Indices idx;
    filterIndices(input, idx);
    pcl::copyPointCloud(input, idx, output);
  }
  void filterIndices(const Cloud & input, Indices & output) const
  {
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::SACSegmentation<PointT> seg;
    seg.setOptimizeCoefficients(params_.optimize_coefficients);
    seg.setModelType(pcl::SACMODEL_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(params_.distance_threshold);
    seg.setMaxIterations(std::max(params_.max_iterations, 1));
    seg.setInputCloud(common::makeConstPtr(input));
    seg.segment(*inliers, *coefficients);
    output = inliers->indices;
    std::sort(output.begin(), output.end());
    if (params_.invert)
    {
      std::vector<bool> selected(input.size(), false);
      for (int i: output)
      {
        if (i >= 0)
        {
          selected[static_cast<std::size_t>(i)] = true;
        }
      }
      output.clear();
      for (std::size_t i = 0; i < input.size(); ++i)
      {
        if (!selected[i])
        {
          output.push_back(static_cast<int>(i));
        }
      }
    }
  }
private:
  Params params_{0.05, 100, true, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__SAC_SEGMENTATION_EXTRACT_FILTER_HPP_
