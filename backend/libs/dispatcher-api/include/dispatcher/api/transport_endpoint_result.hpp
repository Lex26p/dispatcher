#pragma once

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_endpoint_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class TransportEndpointResult
    {
    public:
        [[nodiscard]] static TransportEndpointResult success(
            TransportEndpointStatus status,
            TransportEndpoint endpoint,
            std::string message = {}
        );

        [[nodiscard]] static TransportEndpointResult failure(
            TransportEndpointStatus status,
            std::string message = {},
            std::string field = {},
            std::string value = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TransportEndpointStatus status() const noexcept;

        [[nodiscard]] bool has_endpoint() const noexcept;

        [[nodiscard]] const TransportEndpoint& endpoint() const;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& value() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_value() const noexcept;

    private:
        TransportEndpointResult(
            TransportEndpointStatus status,
            std::optional<TransportEndpoint> endpoint,
            std::string message,
            std::string field,
            std::string value
        );

        TransportEndpointStatus status_{
            TransportEndpointStatus::InvalidEndpoint
        };
        std::optional<TransportEndpoint> endpoint_;
        std::string message_;
        std::string field_;
        std::string value_;
    };
}