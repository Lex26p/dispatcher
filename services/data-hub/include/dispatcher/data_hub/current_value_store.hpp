#pragma once

#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace dispatcher::data_hub {

class CurrentValueStore final {
public:
    using MetricSample = v1::MetricSample;

    // Stores sample as the current value for its metric.
    //
    // Returns false when the sample has no usable metric id or no value.
    // A successful write replaces the previous current value for the same id.
    bool put(const MetricSample& sample);

    // Returns a copy of the current sample for metric_id, or std::nullopt when
    // no current value has been published for that id.
    [[nodiscard]] std::optional<MetricSample> get(std::string_view metric_id) const;

    [[nodiscard]] std::size_t size() const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, MetricSample> values_;
};

}  // namespace dispatcher::data_hub
