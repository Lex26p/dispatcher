#pragma once

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_router_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class TransportRouterResult
    {
    public:
        [[nodiscard]] static TransportRouterResult handled(
            TransportResponse response,
            TransportEndpoint endpoint,
            std::string message = {}
        );

        [[nodiscard]] static TransportRouterResult failed(
            TransportRouterStatus status,
            TransportResponse response,
            std::string reason = {},
            std::string field = {},
            std::string value = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TransportRouterStatus status() const noexcept;

        [[nodiscard]] bool has_response() const noexcept;

        [[nodiscard]] const TransportResponse& response() const;

        [[nodiscard]] bool has_endpoint() const noexcept;

        [[nodiscard]] const TransportEndpoint& endpoint() const;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& reason() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& value() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_reason() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_value() const noexcept;

    private:
        TransportRouterResult(
            TransportRouterStatus status,
            std::optional<TransportResponse> response,
            std::optional<TransportEndpoint> endpoint,
            std::string message,
            std::string reason,
            std::string field,
            std::string value
        );

        TransportRouterStatus status_{ TransportRouterStatus::HandlerFailed };
        std::optional<TransportResponse> response_;
        std::optional<TransportEndpoint> endpoint_;
        std::string message_;
        std::string reason_;
        std::string field_;
        std::string value_;
    };
}