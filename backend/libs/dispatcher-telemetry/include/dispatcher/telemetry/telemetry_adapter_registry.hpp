#pragma once

#include <dispatcher/telemetry/telemetry_adapter.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::telemetry
{
    class TelemetryAdapterRegistry
    {
    public:
        using AdapterPtr = std::shared_ptr<TelemetryAdapter>;

        [[nodiscard]] TelemetryAdapterResult register_adapter(
            AdapterPtr adapter
        );

        [[nodiscard]] TelemetryAdapterResult unregister_adapter(
            const std::string& name
        );

        [[nodiscard]] AdapterPtr find(
            const std::string& name
        ) const;

        [[nodiscard]] bool contains(
            const std::string& name
        ) const;

        [[nodiscard]] std::vector<std::string> names() const;

        [[nodiscard]] std::vector<AdapterPtr> adapters() const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::unordered_map<std::string, AdapterPtr> adapters_by_name_;
    };
}