#include "dispatcher/data_hub/current_value_store.hpp"

#include <mutex>
#include <shared_mutex>
#include <string>

namespace dispatcher::data_hub {

bool CurrentValueStore::put(const MetricSample& sample) {
    if (!sample.has_metric_id() || sample.metric_id().value().empty()) {
        return false;
    }

    if (!sample.has_value() || sample.value().kind_case() == v1::MetricValue::KIND_NOT_SET) {
        return false;
    }

    std::unique_lock lock(mutex_);
    values_.insert_or_assign(sample.metric_id().value(), sample);
    return true;
}

std::optional<CurrentValueStore::MetricSample> CurrentValueStore::get(
    const std::string_view metric_id) const {
    if (metric_id.empty()) {
        return std::nullopt;
    }

    std::shared_lock lock(mutex_);
    const auto found = values_.find(std::string(metric_id));

    if (found == values_.end()) {
        return std::nullopt;
    }

    return found->second;
}

std::size_t CurrentValueStore::size() const {
    std::shared_lock lock(mutex_);
    return values_.size();
}

}  // namespace dispatcher::data_hub
