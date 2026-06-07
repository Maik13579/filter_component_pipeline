// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__EUCLIDEAN_CLUSTER_EXTRACT_FILTER_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__EUCLIDEAN_CLUSTER_EXTRACT_FILTER_HPP_

#include <algorithm>
#include <vector>
#include <pcl/common/io.h>
#include <pcl/point_cloud.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include "pcl_filter_components/filters/common/filter_utils.hpp"

namespace pcl_filter_components::filters::segmentation
{
template <typename PointT>
class EuclideanClusterExtractFilter
{
public:
  using Cloud = pcl::PointCloud<PointT>;
  using Indices = std::vector<int>;
  struct Params
  {
    double cluster_tolerance;
    int min_cluster_size;
    int max_cluster_size;
    int cluster_index;
    bool invert;
  };
  static const char * nodeName()
  {
    return "euclidean_cluster_extract_filter";
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
    typename pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    tree->setInputCloud(common::makeConstPtr(input));
    std::vector<pcl::PointIndices> clusters;
    pcl::EuclideanClusterExtraction<PointT> extractor;
    extractor.setClusterTolerance(params_.cluster_tolerance);
    extractor.setMinClusterSize(std::max(params_.min_cluster_size, 1));
    extractor.setMaxClusterSize(std::max(params_.max_cluster_size, params_.min_cluster_size));
    extractor.setSearchMethod(tree);
    extractor.setInputCloud(common::makeConstPtr(input));
    extractor.extract(clusters);
    std::sort(clusters.begin(), clusters.end(), [](const auto & a, const auto & b) {
        return a.indices.size() > b.indices.size();
      }
              );
    output.clear();
    if (!clusters.empty())
    {
      const auto selected = std::clamp(params_.cluster_index, 0, static_cast<int>(clusters.size()) - 1);
      output = clusters[static_cast<std::size_t>(selected)].indices;
      std::sort(output.begin(), output.end());
    }
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
  Params params_{0.2, 1, 1000000, 0, false};
};
}

#endif  // PCL_FILTER_COMPONENTS__FILTERS__SEGMENTATION__EUCLIDEAN_CLUSTER_EXTRACT_FILTER_HPP_
