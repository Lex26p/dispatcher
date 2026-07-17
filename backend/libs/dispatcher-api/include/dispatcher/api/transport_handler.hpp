#pragma once

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_response.hpp>

#include <functional>

namespace dispatcher::api
{
    class TransportHandler
    {
    public:
        using HandlerFunction =
            std::function<TransportResponse(const TransportRequest&)>;

        TransportHandler(
            TransportEndpoint endpoint,
            HandlerFunction handler,
            bool enabled = true
        );

        [[nodiscard]] const TransportEndpoint& endpoint() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] bool disabled() const noexcept;

        [[nodiscard]] bool has_handler() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool matches(
            TransportMethod method,
            const std::string& path
        ) const noexcept;

        [[nodiscard]] bool compatible_with(
            TransportProtocol protocol
        ) const noexcept;

        [[nodiscard]] TransportResponse handle(
            const TransportRequest& request
        ) const;

    private:
        TransportEndpoint endpoint_;
        HandlerFunction handler_;
        bool enabled_{ true };
    };
}