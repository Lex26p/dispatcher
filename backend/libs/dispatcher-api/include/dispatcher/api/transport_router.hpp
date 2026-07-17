#pragma once

#include <dispatcher/api/transport_endpoint_registry.hpp>
#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_router_result.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::api
{
    class TransportRouter
    {
    public:
        [[nodiscard]] TransportEndpointResult add_handler(
            TransportHandler handler
        );

        [[nodiscard]] TransportEndpointResult remove_handler(
            TransportMethod method,
            const std::string& path
        );

        [[nodiscard]] TransportRouterResult dispatch(
            const TransportRequest& request
        ) const;

        [[nodiscard]] std::optional<TransportHandler> find_handler(
            TransportMethod method,
            const std::string& path
        ) const;

        [[nodiscard]] bool contains_handler(
            TransportMethod method,
            const std::string& path
        ) const;

        [[nodiscard]] const TransportEndpointRegistry& endpoint_registry()
            const noexcept;

        [[nodiscard]] std::vector<TransportEndpoint> endpoints() const;

        [[nodiscard]] std::size_t handler_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        TransportEndpointRegistry endpoint_registry_;
        std::unordered_map<std::string, TransportHandler> handlers_by_key_;
    };
}