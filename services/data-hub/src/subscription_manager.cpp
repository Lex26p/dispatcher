#include "dispatcher/data_hub/subscription_manager.hpp"

#include <utility>
#include <vector>

namespace dispatcher::data_hub {

Subscription::Subscription(std::vector<std::string> metric_ids)
    : metric_ids_(metric_ids.begin(), metric_ids.end()) {}

bool Subscription::matches(const std::string_view metric_id) const {
    return metric_ids_.find(std::string(metric_id)) != metric_ids_.end();
}

void Subscription::push(const MetricSample& sample) {
    std::lock_guard lock(mutex_);

    if (closed_) {
        return;
    }

    pending_.push_back(sample);
    condition_.notify_one();
}

Subscription::WaitResult Subscription::wait_next(
    MetricSample& sample,
    const std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);

    const bool ready = condition_.wait_for(
        lock,
        timeout,
        [this] {
            return closed_ || !pending_.empty();
        });

    if (!ready) {
        return WaitResult::timeout;
    }

    if (!pending_.empty()) {
        sample = std::move(pending_.front());
        pending_.pop_front();
        return WaitResult::item;
    }

    return WaitResult::closed;
}

void Subscription::close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    condition_.notify_all();
}

SubscriptionManager::Handle SubscriptionManager::create(
    std::vector<std::string> metric_ids) {
    auto subscription =
        std::make_shared<Subscription>(std::move(metric_ids));

    std::lock_guard lock(mutex_);
    const auto id = next_id_++;
    subscriptions_.emplace(id, subscription);

    return {
        .id = id,
        .subscription = std::move(subscription),
    };
}

void SubscriptionManager::remove(const std::uint64_t id) {
    std::shared_ptr<Subscription> subscription;

    {
        std::lock_guard lock(mutex_);
        const auto found = subscriptions_.find(id);

        if (found == subscriptions_.end()) {
            return;
        }

        subscription = std::move(found->second);
        subscriptions_.erase(found);
    }

    subscription->close();
}

void SubscriptionManager::publish(const v1::MetricSample& sample) {
    std::vector<std::shared_ptr<Subscription>> matching;

    {
        std::lock_guard lock(mutex_);

        for (const auto& [id, subscription] : subscriptions_) {
            (void)id;

            if (subscription->matches(sample.metric_id().value())) {
                matching.push_back(subscription);
            }
        }
    }

    for (const auto& subscription : matching) {
        subscription->push(sample);
    }
}

}  // namespace dispatcher::data_hub
