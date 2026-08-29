#pragma once

#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dispatcher::data_hub {

class Subscription final {
public:
    using MetricSample = v1::MetricSample;

    enum class WaitResult {
        item,
        timeout,
        closed,
    };

    explicit Subscription(std::vector<std::string> metric_ids);

    [[nodiscard]] bool matches(std::string_view metric_id) const;

    void push(const MetricSample& sample);

    WaitResult wait_next(
        MetricSample& sample,
        std::chrono::milliseconds timeout);

    void close();

private:
    std::unordered_set<std::string> metric_ids_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<MetricSample> pending_;
    bool closed_{false};
};

class SubscriptionManager final {
public:
    struct Handle {
        std::uint64_t id;
        std::shared_ptr<Subscription> subscription;
    };

    [[nodiscard]] Handle create(std::vector<std::string> metric_ids);
    void remove(std::uint64_t id);
    void publish(const v1::MetricSample& sample);

private:
    std::mutex mutex_;
    std::unordered_map<std::uint64_t, std::shared_ptr<Subscription>> subscriptions_;
    std::uint64_t next_id_{1};
};

}  // namespace dispatcher::data_hub
