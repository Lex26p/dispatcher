#include <dispatcher/telemetry/telemetry_adapter_registry.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::telemetry
{
    TelemetryAdapterResult TelemetryAdapterRegistry::register_adapter(
        AdapterPtr adapter
    )
    {
        if (!adapter)
        {
            return TelemetryAdapterResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_registry.register",
                "telemetry-adapter-registry",
                {},
                "adapter",
                "adapter pointer is null"
            );
        }

        auto name = adapter->name();

        if (name.empty())
        {
            return TelemetryAdapterResult::failure(
                TelemetryAdapterStatus::InvalidConfiguration,
                "telemetry_adapter_registry.register",
                "telemetry-adapter-registry",
                {},
                "name",
                "adapter name is empty"
            );
        }

        if (adapters_by_name_.contains(name))
        {
            return TelemetryAdapterResult::failure(
                TelemetryAdapterStatus::InvalidConfiguration,
                "telemetry_adapter_registry.register",
                "telemetry-adapter-registry",
                name,
                "name",
                "adapter name is already registered"
            );
        }

        adapters_by_name_.emplace(
            std::move(name),
            std::move(adapter)
        );

        return TelemetryAdapterResult::success();
    }

    TelemetryAdapterResult TelemetryAdapterRegistry::unregister_adapter(
        const std::string& name
    )
    {
        if (name.empty())
        {
            return TelemetryAdapterResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_registry.unregister",
                "telemetry-adapter-registry",
                {},
                "name",
                "adapter name is empty"
            );
        }

        const auto erased_count = adapters_by_name_.erase(name);

        if (erased_count == 0)
        {
            return TelemetryAdapterResult::failure(
                TelemetryAdapterStatus::InvalidRequest,
                "telemetry_adapter_registry.unregister",
                "telemetry-adapter-registry",
                name,
                "name",
                "adapter is not registered"
            );
        }

        return TelemetryAdapterResult::success();
    }

    TelemetryAdapterRegistry::AdapterPtr TelemetryAdapterRegistry::find(
        const std::string& name
    ) const
    {
        const auto iterator = adapters_by_name_.find(name);

        if (iterator == adapters_by_name_.end())
        {
            return {};
        }

        return iterator->second;
    }

    bool TelemetryAdapterRegistry::contains(
        const std::string& name
    ) const
    {
        return adapters_by_name_.contains(name);
    }

    std::vector<std::string> TelemetryAdapterRegistry::names() const
    {
        std::vector<std::string> result;

        result.reserve(adapters_by_name_.size());

        for (const auto& [name, adapter] : adapters_by_name_)
        {
            result.push_back(name);
        }

        std::sort(
            result.begin(),
            result.end()
        );

        return result;
    }

    std::vector<TelemetryAdapterRegistry::AdapterPtr>
        TelemetryAdapterRegistry::adapters() const
    {
        std::vector<AdapterPtr> result;

        result.reserve(adapters_by_name_.size());

        for (const auto& [name, adapter] : adapters_by_name_)
        {
            result.push_back(adapter);
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const AdapterPtr& left, const AdapterPtr& right)
            {
                return left->name() < right->name();
            }
        );

        return result;
    }

    std::size_t TelemetryAdapterRegistry::size() const noexcept
    {
        return adapters_by_name_.size();
    }

    bool TelemetryAdapterRegistry::empty() const noexcept
    {
        return adapters_by_name_.empty();
    }

    void TelemetryAdapterRegistry::clear() noexcept
    {
        adapters_by_name_.clear();
    }
}