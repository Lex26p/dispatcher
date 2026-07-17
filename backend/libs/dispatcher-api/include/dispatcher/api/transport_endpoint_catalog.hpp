#pragma once

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_endpoint_registry.hpp>
#include <dispatcher/api/transport_endpoint_result.hpp>
#include <dispatcher/api/transport_method.hpp>

#include <string>
#include <vector>

namespace dispatcher::api
{
    class TransportEndpointCatalog
    {
    public:
        [[nodiscard]] static std::vector<TransportEndpoint>
            standard_endpoints();

        [[nodiscard]] static std::vector<TransportEndpoint>
            public_endpoints();

        [[nodiscard]] static std::vector<TransportEndpoint>
            authenticated_endpoints();

        [[nodiscard]] static std::vector<TransportEndpoint>
            streaming_endpoints();

        [[nodiscard]] static TransportEndpointRegistry build_registry();

        [[nodiscard]] static std::vector<TransportEndpointResult>
            register_standard_endpoints(
                TransportEndpointRegistry& registry
            );

        [[nodiscard]] static bool is_standard_endpoint(
            TransportMethod method,
            const std::string& path
        );

        [[nodiscard]] static bool is_public_endpoint(
            TransportMethod method,
            const std::string& path
        );

        [[nodiscard]] static bool is_streaming_endpoint(
            TransportMethod method,
            const std::string& path
        );
    };
}