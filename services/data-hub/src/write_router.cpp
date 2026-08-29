#include "dispatcher/data_hub/write_router.hpp"

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace dispatcher::data_hub {

bool WriteRouter::register_provider(
    std::string metric_id,
    std::shared_ptr<MetricWriteProvider> provider) {
    if (metric_id.empty() || provider == nullptr) {
        return false;
    }

    std::unique_lock lock(mutex_);

    const auto [iterator, inserted] =
        providers_.emplace(std::move(metric_id), std::move(provider));

    (void)iterator;
    return inserted;
}

WriteRouter::DispatchResult WriteRouter::dispatch(
    const v1::WriteMetricRequest& request) const {
    std::shared_ptr<MetricWriteProvider> provider;

    {
        std::shared_lock lock(mutex_);

        const auto found =
            providers_.find(request.metric_id().value());

        if (found == providers_.end()) {
            return DispatchResult::no_provider;
        }

        provider = found->second;
    }

    return provider->write(request)
        ? DispatchResult::accepted
        : DispatchResult::rejected;
}

}  // namespace dispatcher::data_hub
