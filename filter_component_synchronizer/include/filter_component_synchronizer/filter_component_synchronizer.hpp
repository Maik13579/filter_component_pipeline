// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#ifndef FILTER_COMPONENT_SYNCHRONIZER__FILTER_COMPONENT_SYNCHRONIZER_HPP_
#define FILTER_COMPONENT_SYNCHRONIZER__FILTER_COMPONENT_SYNCHRONIZER_HPP_

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

namespace filter_component_synchronizer
{

enum class SyncMode
{
  ReceiptTime,
  Latest,
};

inline SyncMode syncModeFromString(const std::string & value)
{
  if (value == "receipt_time") {
    return SyncMode::ReceiptTime;
  }
  if (value == "latest") {
    return SyncMode::Latest;
  }
  throw std::invalid_argument("Unsupported sync mode '" + value + "'");
}

struct SynchronizerOptions
{
  SyncMode mode{SyncMode::ReceiptTime};
  size_t queue_size{10U};
  double max_interval{0.05};
};

template <typename AdapterT>
using CustomMessage = typename rclcpp::TypeAdapter<AdapterT>::custom_type;

namespace detail
{

inline std::int64_t secondsToNanoseconds(double seconds)
{
  return static_cast<std::int64_t>(std::llround(seconds * 1000000000.0));
}

}  // namespace detail

class FilterComponentSynchronizer
{
public:
  using ReadyCallback = std::function<void()>;

  explicit FilterComponentSynchronizer(
    SynchronizerOptions options = {},
    ReadyCallback ready_callback = {})
  : options_(options),
    ready_callback_(std::move(ready_callback))
  {
    if (options_.queue_size == 0U) {
      options_.queue_size = 1U;
    }
    if (options_.max_interval < 0.0) {
      options_.max_interval = 0.0;
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
    auto clock = node.get_clock();
    holder->subscription = node.template create_subscription<AdapterT>(
      topic_name,
      qos,
      [this, port_name, typed_holder, clock](std::unique_ptr<CustomMessage<AdapterT>> message) {
        this->storeMessage<AdapterT>(
          port_name,
          *typed_holder,
          std::move(message),
          clock->now().nanoseconds());
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
    if (options_.mode == SyncMode::Latest && holder.latest) {
      return std::make_unique<CustomMessage<AdapterT>>(*holder.latest);
    }
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
    virtual bool hasLatest() const = 0;
    virtual std::int64_t stampAt(size_t index) const = 0;
    virtual size_t queuedSize() const = 0;
    virtual void moveQueuedToLatestAndDropThrough(size_t index) = 0;
    virtual void popFront() = 0;
    virtual void trimTo(size_t queue_size) = 0;

    std::type_index adapter_type;
    rclcpp::SubscriptionBase::SharedPtr subscription;
  };

  template <typename AdapterT>
  struct InputHolder : InputConcept
  {
    struct QueuedMessage
    {
      std::int64_t stamp{0};
      std::unique_ptr<CustomMessage<AdapterT>> message;
    };

    InputHolder()
    : InputConcept(std::type_index(typeid(AdapterT)))
    {
    }

    bool hasQueued() const override
    {
      return !queued.empty();
    }

    bool hasLatest() const override
    {
      return latest != nullptr;
    }

    std::int64_t stampAt(size_t index) const override
    {
      return queued.at(index).stamp;
    }

    size_t queuedSize() const override
    {
      return queued.size();
    }

    void moveQueuedToLatestAndDropThrough(size_t index) override
    {
      latest = std::move(queued.at(index).message);
      using DifferenceType =
        typename std::deque<QueuedMessage>::difference_type;
      queued.erase(queued.begin(), queued.begin() + static_cast<DifferenceType>(index + 1U));
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

    std::deque<QueuedMessage> queued;
    std::unique_ptr<CustomMessage<AdapterT>> latest;
  };

  template <typename AdapterT>
  void storeMessage(
    const std::string & port_name,
    InputHolder<AdapterT> & holder,
    std::unique_ptr<CustomMessage<AdapterT>> message,
    std::int64_t receipt_stamp)
  {
    ReadyCallback callback;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      if (inputs_.count(port_name) == 0U) {
        return;
      }
      if (options_.mode == SyncMode::Latest) {
        holder.latest = std::move(message);
      } else {
        holder.queued.push_back({receipt_stamp, std::move(message)});
        holder.trimTo(options_.queue_size);
      }
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

    if (options_.mode == SyncMode::Latest) {
      for (const auto & port_name : input_order_) {
        if (!inputs_.at(port_name)->hasLatest()) {
          return false;
        }
      }
      return true;
    }

    for (const auto & port_name : input_order_) {
      if (!inputs_.at(port_name)->hasQueued()) {
        return false;
      }
    }
    auto selected = std::unordered_map<std::string, size_t>{};
    if (input_order_.size() == 1U) {
      selected[input_order_.front()] = 0U;
    } else if (!selectReceiptTimeLocked(selected)) {
      return false;
    }

    for (const auto & port_name : input_order_) {
      inputs_.at(port_name)->moveQueuedToLatestAndDropThrough(selected.at(port_name));
    }
    return true;
  }

  bool selectReceiptTimeLocked(std::unordered_map<std::string, size_t> & selected)
  {
    auto best_span = std::numeric_limits<std::int64_t>::max();
    auto best_selected = std::unordered_map<std::string, size_t>{};

    for (const auto & port_name : input_order_) {
      const auto & input = *inputs_.at(port_name);
      for (size_t index = 0; index < input.queuedSize(); ++index) {
        auto candidate = std::unordered_map<std::string, size_t>{};
        const auto minimum = input.stampAt(index);
        auto maximum = minimum;
        candidate[port_name] = index;
        auto complete = true;
        for (const auto & other_port_name : input_order_) {
          if (other_port_name == port_name) {
            continue;
          }
          const auto & other = *inputs_.at(other_port_name);
          auto other_index = other.queuedSize();
          for (size_t other_candidate = 0; other_candidate < other.queuedSize(); ++other_candidate) {
            if (other.stampAt(other_candidate) >= minimum) {
              other_index = other_candidate;
              break;
            }
          }
          if (other_index == other.queuedSize()) {
            complete = false;
            break;
          }
          candidate[other_port_name] = other_index;
          maximum = std::max(maximum, other.stampAt(other_index));
        }
        if (complete && maximum - minimum < best_span) {
          best_span = maximum - minimum;
          best_selected = std::move(candidate);
        }
      }
    }

    const auto max_interval_ns = detail::secondsToNanoseconds(options_.max_interval);
    if (!best_selected.empty() && best_span <= max_interval_ns) {
      selected = std::move(best_selected);
      return true;
    }
    dropClearlyOldMessagesLocked(latestQueuedStampLocked(), max_interval_ns);
    return false;
  }

  std::int64_t latestQueuedStampLocked() const
  {
    auto stamp = std::numeric_limits<std::int64_t>::min();
    for (const auto & port_name : input_order_) {
      const auto & input = *inputs_.at(port_name);
      for (size_t index = 0; index < input.queuedSize(); ++index) {
        stamp = std::max(stamp, input.stampAt(index));
      }
    }
    return stamp;
  }

  void dropClearlyOldMessagesLocked(std::int64_t newest_seen_stamp, std::int64_t max_interval_ns)
  {
    const auto oldest_allowed = newest_seen_stamp - max_interval_ns;
    for (const auto & port_name : input_order_) {
      auto & input = *inputs_.at(port_name);
      while (input.queuedSize() > 0U && input.stampAt(0U) < oldest_allowed) {
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

}  // namespace filter_component_synchronizer

#endif  // FILTER_COMPONENT_SYNCHRONIZER__FILTER_COMPONENT_SYNCHRONIZER_HPP_
