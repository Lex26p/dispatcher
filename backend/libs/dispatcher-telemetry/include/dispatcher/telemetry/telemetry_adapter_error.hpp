#pragma once

#include <dispatcher/telemetry/telemetry_adapter_status.hpp>

#include <string>

namespace dispatcher::telemetry
{
    class TelemetryAdapterError
    {
    public:
        TelemetryAdapterError(
            TelemetryAdapterStatus status,
            std::string operation,
            std::string adapter_name = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] TelemetryAdapterStatus status() const noexcept;

        [[nodiscard]] const std::string& operation() const noexcept;

        [[nodiscard]] const std::string& adapter_name() const noexcept;

        [[nodiscard]] const std::string& resource() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] bool has_operation() const noexcept;

        [[nodiscard]] bool has_adapter_name() const noexcept;

        [[nodiscard]] bool has_resource() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool retryable() const noexcept;

    private:
        TelemetryAdapterStatus status_{ TelemetryAdapterStatus::UnknownError };
        std::string operation_;
        std::string adapter_name_;
        std::string resource_;
        std::string field_;
        std::string message_;
    };
}