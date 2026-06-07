// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_COMPONENTS__FILTERS__COMMON__FILTER_UTILS_HPP_
#define PCL_FILTER_COMPONENTS__FILTERS__COMMON__FILTER_UTILS_HPP_

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <pcl/common/point_tests.h>
#include <pcl/point_cloud.h>

namespace pcl_filter_components::filters::common
{

template <typename PointT>
using Cloud = pcl::PointCloud<PointT>;

template <typename PointT>
typename Cloud<PointT>::ConstPtr makeConstPtr(const Cloud<PointT> & cloud)
{
  return typename Cloud<PointT>::ConstPtr(&cloud, [](const Cloud<PointT> *) {});
}

template <typename PointT, typename PredicateT>
void copyIf(const Cloud<PointT> & input, Cloud<PointT> & output, PredicateT predicate)
{
  output.clear();
  output.header = input.header;
  output.reserve(input.size());
  for (const auto & point : input) {
    if (predicate(point)) {
      output.push_back(point);
    }
  }
  output.width = static_cast<std::uint32_t>(output.size());
  output.height = 1U;
  output.is_dense = input.is_dense;
}

template <typename PointT, typename PredicateT>
void indicesIf(const Cloud<PointT> & input, std::vector<int> & output, PredicateT predicate)
{
  output.clear();
  for (std::size_t i = 0; i < input.size(); ++i) {
    if (predicate(input[i])) {
      output.push_back(static_cast<int>(i));
    }
  }
}

template <typename PointT, typename = void>
struct HasIntensity : std::false_type {};

template <typename PointT>
struct HasIntensity<PointT, std::void_t<decltype(std::declval<PointT>().intensity)>> : std::true_type {};

template <typename PointT, typename = void>
struct HasColor : std::false_type {};

template <typename PointT>
struct HasColor<PointT, std::void_t<decltype(std::declval<PointT>().rgba)>> : std::true_type {};

template <typename PointT>
float intensity(const PointT & point)
{
  if constexpr (HasIntensity<PointT>::value) {
    return point.intensity;
  }
  return 0.0F;
}

template <typename PointT>
std::uint8_t colorChannel(const PointT & point, int shift)
{
  if constexpr (HasColor<PointT>::value) {
    return static_cast<std::uint8_t>((point.rgba >> shift) & 0xffU);
  }
  return 0U;
}

}  // namespace pcl_filter_components::filters::common

#endif  // PCL_FILTER_COMPONENTS__FILTERS__COMMON__FILTER_UTILS_HPP_
