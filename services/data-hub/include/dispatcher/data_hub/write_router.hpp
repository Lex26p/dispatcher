#pragma once

#include "dispatcher/data_hub/v1/data_hub.pb.h"

#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace dispatcher::data_hub {

class MetricWriteProvider {
public:
    virtual ~MetricWriteProvider() = default;

    // Returns true when the provider accepted the write request for further
    // processing. Acceptance does not mean that equipment has already applied
    // the value or that Data Hub should change its current value.
    virtual bool write(const v1::WriteMetricRequest& request) = 0;
};

class WriteRouter final {
public:
    enum class DispatchResult {
        accepted,
        no_provider,
        rejected,
    };

    // Registers one current provider for metric_id.
    //
    // Returns false for an empty id, null provider, or when another provider
    // is already registered for the same metric id.
    bool register_provider(
        std::string metric_id,
        std::shared_ptr<MetricWriteProvider> provider);

    [[nodiscard]] DispatchResult dispatch(
        const v1::WriteMetricRequest& request) const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<
        std::string,
        std::shared_ptr<MetricWriteProvider>> providers_;
};

}  // namespace dispatcher::data_hub
