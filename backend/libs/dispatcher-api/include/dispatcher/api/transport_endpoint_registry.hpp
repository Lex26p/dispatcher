#pragma once

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_endpoint_result.hpp>
#include <dispatcher/api/transport_method.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dispatcher::api
{
    class TransportEndpointRegistry
    {
    public:
        [[nodiscard]] TransportEndpointResult add(
            TransportEndpoint endpoint
        );

        [[nodiscard]] TransportEndpointResult remove(
            TransportMethod method,
            const std::string& path
        );

        [[nodiscard]] TransportEndpointResult resolve(
            TransportMethod method,
            const std::string& path
        ) const;

        [[nodiscard]] std::optional<TransportEndpoint> find(
            TransportMethod method,
            const std::string& path
        ) const;

        [[nodiscard]] bool contains(
            TransportMethod method,
            const std::string& path
        ) const;

        [[nodiscard]] std::vector<TransportEndpoint> endpoints() const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::unordered_map<std::string, TransportEndpoint> endpoints_by_key_;
    };
}