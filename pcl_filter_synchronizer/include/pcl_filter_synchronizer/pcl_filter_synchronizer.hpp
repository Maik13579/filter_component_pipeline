// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef PCL_FILTER_SYNCHRONIZER__PCL_FILTER_SYNCHRONIZER_HPP_
#define PCL_FILTER_SYNCHRONIZER__PCL_FILTER_SYNCHRONIZER_HPP_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rclcpp/rclcpp.hpp>

namespace pcl_filter_synchronizer
{

enum class SyncPolicy
{
  ExactTime,
  ApproximateTime,
};

inline SyncPolicy syncPolicyFromString(const std::string & value)
{
  if (value == "ExactTime") {
    return SyncPolicy::ExactTime;
  }
  if (value == "ApproximateTime") {
    return SyncPolicy::ApproximateTime;
  }
  throw std::invalid_argument("Unsupported sync policy '" + value + "'");
}

struct SynchronizerOptions
{
  SyncPolicy policy{SyncPolicy::ExactTime};
  size_t queue_size{10U};
  double slop{0.05};
};

template <typename AdapterT>
using CustomMessage = typename rclcpp::TypeAdapter<AdapterT>::custom_type;

class PclFilterSynchronizer
{
public:
  using ReadyCallback = std::function<void()>;

  explicit PclFilterSynchronizer(
    SynchronizerOptions options = {},
    ReadyCallback ready_callback = {})
  : options_(options),
    ready_callback_(std::move(ready_callback))
  {
    if (options_.queue_size == 0U) {
      options_.queue_size = 1U;
    }
    if (options_.slop < 0.0) {
      options_.slop = 0.0;
    }
  }

  template <typename NodeT, typename AdapterT>
  void addInput(
    NodeT & node,
    const std::string & port_name,
    const std::string & topic_name,
    const rclcpp::QoS & qos)
  {
    if (inputs_.count(port_name) != 0U) {
      throw std::invalid_argument("Duplicate input port '" + port_name + "'");
    }

    auto holder = std::make_unique<InputHolder<AdapterT>>();
    auto * typed_holder = holder.get();
    holder->subscription = node.template create_subscription<AdapterT>(
      topic_name,
      qos,
      [this, port_name, typed_holder](std::unique_ptr<CustomMessage<AdapterT>> message) {
        this->storeMessage<AdapterT>(port_name, *typed_holder, std::move(message));
      });
    input_order_.push_back(port_name);
    inputs_.emplace(port_name, std::move(holder));
  }

  void clear()
  {
    std::lock_guard<std::mutex> lock{mutex_};
    input_order_.clear();
    inputs_.clear();
  }

  template <typename AdapterT>
  std::unique_ptr<CustomMessage<AdapterT>> takeInput(const std::string & port_name)
  {
    std::lock_guard<std::mutex> lock{mutex_};
    auto & holder = getTypedHolder<AdapterT>(port_name);
    return std::move(holder.latest);
  }

  template <typename AdapterT>
  const CustomMessage<AdapterT> * peekInput(const std::string & port_name) const
  {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto & holder = getTypedHolder<AdapterT>(port_name);
    return holder.latest.get();
  }

private:
  struct InputConcept
  {
    explicit InputConcept(std::type_index adapter_type_in)
    : adapter_type(adapter_type_in)
    {
    }

    virtual ~InputConcept() = default;
    virtual bool hasQueued() const = 0;
    virtual std::uint64_t stampAt(size_t index) const = 0;
    virtual size_t queuedSize() const = 0;
    virtual void moveQueuedToLatest(size_t index) = 0;
    virtual void popFront() = 0;
    virtual void trimTo(size_t queue_size) = 0;

    std::type_index adapter_type;
    rclcpp::SubscriptionBase::SharedPtr subscription;
  };

  template <typename AdapterT>
  struct InputHolder : InputConcept
  {
    InputHolder()
    : InputConcept(std::type_index(typeid(AdapterT)))
    {
    }

    bool hasQueued() const override
    {
      return !queued.empty();
    }

    std::uint64_t stampAt(size_t index) const override
    {
      return queued.at(index)->header.stamp;
    }

    size_t queuedSize() const override
    {
      return queued.size();
    }

    void moveQueuedToLatest(size_t index) override
    {
      latest = std::move(queued.at(index));
      using DifferenceType =
        typename std::deque<std::unique_ptr<CustomMessage<AdapterT>>>::difference_type;
      queued.erase(queued.begin() + static_cast<DifferenceType>(index));
    }

    void popFront() override
    {
      queued.pop_front();
    }

    void trimTo(size_t queue_size) override
    {
      while (queued.size() > queue_size) {
        queued.pop_front();
      }
    }

    std::deque<std::unique_ptr<CustomMessage<AdapterT>>> queued;
    std::unique_ptr<CustomMessage<AdapterT>> latest;
  };

  template <typename AdapterT>
  void storeMessage(
    const std::string & port_name,
    InputHolder<AdapterT> & holder,
    std::unique_ptr<CustomMessage<AdapterT>> message)
  {
    ReadyCallback callback;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (inputs_.count(port_name) == 0U) {
        return;
      }
      holder.queued.push_back(std::move(message));
      holder.trimTo(options_.queue_size);
      if (trySynchronizeLocked()) {
        callback = ready_callback_;
      }
    }
    if (callback) {
      callback();
    }
  }

  bool trySynchronizeLocked()
  {
    if (input_order_.empty()) {
      return false;
    }
    for (const auto & port_name : input_order_) {
      if (!inputs_.at(port_name)->hasQueued()) {
        return false;
      }
    }

    auto selected = std::unordered_map<std::string, size_t>{};
    if (input_order_.size() == 1U) {
      selected[input_order_.front()] = 0U;
    } else if (options_.policy == SyncPolicy::ExactTime) {
      if (!selectExactLocked(selected)) {
        return false;
      }
    } else if (!selectApproximateLocked(selected)) {
      return false;
    }

    for (const auto & port_name : input_order_) {
      inputs_.at(port_name)->moveQueuedToLatest(selected.at(port_name));
    }
    return true;
  }

  bool selectExactLocked(std::unordered_map<std::string, size_t> & selected)
  {
    while (allHaveQueuedLocked()) {
      const auto minmax = queuedStampRangeLocked();
      if (minmax.first == minmax.second) {
        for (const auto & port_name : input_order_) {
          selected[port_name] = 0U;
        }
        return true;
      }
      for (const auto & port_name : input_order_) {
        auto & input = *inputs_.at(port_name);
        if (input.stampAt(0U) == minmax.first) {
          input.popFront();
        }
      }
    }
    return false;
  }

  bool selectApproximateLocked(std::unordered_map<std::string, size_t> & selected)
  {
    const auto reference_stamp = latestQueuedStampLocked();
    const auto slop_us = static_cast<std::uint64_t>(std::llround(options_.slop * 1000000.0));

    for (const auto & port_name : input_order_) {
      const auto & input = *inputs_.at(port_name);
      auto best_index = input.queuedSize();
      auto best_delta = std::numeric_limits<std::uint64_t>::max();
      for (size_t index = 0; index < input.queuedSize(); ++index) {
        const auto stamp = input.stampAt(index);
        const auto delta = stamp > reference_stamp ? stamp - reference_stamp : reference_stamp - stamp;
        if (delta <= slop_us && delta < best_delta) {
          best_index = index;
          best_delta = delta;
        }
      }
      if (best_index == input.queuedSize()) {
        dropClearlyOldMessagesLocked(reference_stamp, slop_us);
        return false;
      }
      selected[port_name] = best_index;
    }
    return true;
  }

  bool allHaveQueuedLocked() const
  {
    return std::all_of(
      input_order_.begin(),
      input_order_.end(),
      [this](const auto & port_name) {return inputs_.at(port_name)->hasQueued();});
  }

  std::pair<std::uint64_t, std::uint64_t> queuedStampRangeLocked() const
  {
    auto minimum = inputs_.at(input_order_.front())->stampAt(0U);
    auto maximum = minimum;
    for (const auto & port_name : input_order_) {
      const auto stamp = inputs_.at(port_name)->stampAt(0U);
      minimum = std::min(minimum, stamp);
      maximum = std::max(maximum, stamp);
    }
    return {minimum, maximum};
  }

  std::uint64_t latestQueuedStampLocked() const
  {
    std::uint64_t stamp = 0U;
    for (const auto & port_name : input_order_) {
      const auto & input = *inputs_.at(port_name);
      for (size_t index = 0; index < input.queuedSize(); ++index) {
        stamp = std::max(stamp, input.stampAt(index));
      }
    }
    return stamp;
  }

  void dropClearlyOldMessagesLocked(std::uint64_t reference_stamp, std::uint64_t slop_us)
  {
    for (const auto & port_name : input_order_) {
      auto & input = *inputs_.at(port_name);
      while (
        input.queuedSize() > 1U &&
        input.stampAt(0U) + slop_us < reference_stamp)
      {
        input.popFront();
      }
    }
  }

  template <typename AdapterT>
  InputHolder<AdapterT> & getTypedHolder(const std::string & port_name) const
  {
    const auto iter = inputs_.find(port_name);
    if (iter == inputs_.end()) {
      throw std::out_of_range("Unknown input port '" + port_name + "'");
    }
    if (iter->second->adapter_type != std::type_index(typeid(AdapterT))) {
      throw std::invalid_argument("Input port '" + port_name + "' has a different adapter type");
    }
    return *static_cast<InputHolder<AdapterT> *>(iter->second.get());
  }

  mutable std::mutex mutex_;
  SynchronizerOptions options_;
  ReadyCallback ready_callback_;
  std::vector<std::string> input_order_;
  std::unordered_map<std::string, std::unique_ptr<InputConcept>> inputs_;
};

}  // namespace pcl_filter_synchronizer

#endif  // PCL_FILTER_SYNCHRONIZER__PCL_FILTER_SYNCHRONIZER_HPP_
