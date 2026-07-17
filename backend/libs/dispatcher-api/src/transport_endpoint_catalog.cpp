#include <dispatcher/api/transport_endpoint_catalog.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::api
{
    namespace
    {
        [[nodiscard]] TransportEndpoint make_catalog_endpoint(
            std::string name,
            TransportMethod method,
            std::string path,
            bool requires_authentication,
            bool streaming,
            std::string description
        )
        {
            return TransportEndpoint(
                std::move(name),
                method,
                std::move(path),
                requires_authentication,
                streaming,
                std::move(description)
            );
        }
    }

    std::vector<TransportEndpoint>
        TransportEndpointCatalog::standard_endpoints()
    {
        return {
            make_catalog_endpoint(
                "health check",
                TransportMethod::Get,
                "/health",
                false,
                false,
                "Public service health check"
            ),
            make_catalog_endpoint(
                "runtime snapshot",
                TransportMethod::Get,
                "/api/v1/runtime",
                true,
                false,
                "Current dispatcher runtime snapshot"
            ),
            make_catalog_endpoint(
                "runtime stream",
                TransportMethod::Get,
                "/api/v1/runtime/stream",
                true,
                true,
                "Streaming dispatcher runtime updates"
            ),
            make_catalog_endpoint(
                "alarm snapshot",
                TransportMethod::Get,
                "/api/v1/alarms",
                true,
                false,
                "Current alarm state snapshot"
            ),
            make_catalog_endpoint(
                "acknowledge alarm",
                TransportMethod::Post,
                "/api/v1/alarms/acknowledgements",
                true,
                false,
                "Acknowledge one or more alarms"
            ),
            make_catalog_endpoint(
                "suppress alarm",
                TransportMethod::Post,
                "/api/v1/alarms/suppressions",
                true,
                false,
                "Create alarm suppression, shelving, or inhibition"
            ),
            make_catalog_endpoint(
                "clear alarm suppression",
                TransportMethod::Delete,
                "/api/v1/alarms/suppressions",
                true,
                false,
                "Clear alarm suppression, shelving, or inhibition"
            ),
            make_catalog_endpoint(
                "export configuration",
                TransportMethod::Get,
                "/api/v1/configuration",
                true,
                false,
                "Export dispatcher configuration"
            ),
            make_catalog_endpoint(
                "import configuration",
                TransportMethod::Post,
                "/api/v1/configuration/import",
                true,
                false,
                "Import dispatcher configuration"
            ),
            make_catalog_endpoint(
                "current operator",
                TransportMethod::Get,
                "/api/v1/operators/me",
                true,
                false,
                "Return authenticated operator context"
            ),
            make_catalog_endpoint(
                "notification routes",
                TransportMethod::Get,
                "/api/v1/notifications/routes",
                true,
                false,
                "List notification routing rules"
            ),
            make_catalog_endpoint(
                "create notification route",
                TransportMethod::Post,
                "/api/v1/notifications/routes",
                true,
                false,
                "Create notification routing rule"
            )
        };
    }

    std::vector<TransportEndpoint>
        TransportEndpointCatalog::public_endpoints()
    {
        std::vector<TransportEndpoint> result;

        for (const auto& endpoint : standard_endpoints())
        {
            if (endpoint.public_endpoint())
            {
                result.push_back(endpoint);
            }
        }

        return result;
    }

    std::vector<TransportEndpoint>
        TransportEndpointCatalog::authenticated_endpoints()
    {
        std::vector<TransportEndpoint> result;

        for (const auto& endpoint : standard_endpoints())
        {
            if (endpoint.requires_authentication())
            {
                result.push_back(endpoint);
            }
        }

        return result;
    }

    std::vector<TransportEndpoint>
        TransportEndpointCatalog::streaming_endpoints()
    {
        std::vector<TransportEndpoint> result;

        for (const auto& endpoint : standard_endpoints())
        {
            if (endpoint.streaming())
            {
                result.push_back(endpoint);
            }
        }

        return result;
    }

    TransportEndpointRegistry TransportEndpointCatalog::build_registry()
    {
        TransportEndpointRegistry registry;

        [[maybe_unused]] const auto results =
            register_standard_endpoints(registry);

        return registry;
    }

    std::vector<TransportEndpointResult>
        TransportEndpointCatalog::register_standard_endpoints(
            TransportEndpointRegistry& registry
        )
    {
        std::vector<TransportEndpointResult> results;

        const auto endpoints = standard_endpoints();

        results.reserve(endpoints.size());

        for (const auto& endpoint : endpoints)
        {
            results.push_back(
                registry.add(endpoint)
            );
        }

        return results;
    }

    bool TransportEndpointCatalog::is_standard_endpoint(
        TransportMethod method,
        const std::string& path
    )
    {
        const auto endpoints = standard_endpoints();

        return std::any_of(
            endpoints.begin(),
            endpoints.end(),
            [&](const TransportEndpoint& endpoint)
            {
                return endpoint.matches(
                    method,
                    path
                );
            }
        );
    }

    bool TransportEndpointCatalog::is_public_endpoint(
        TransportMethod method,
        const std::string& path
    )
    {
        const auto endpoints = public_endpoints();

        return std::any_of(
            endpoints.begin(),
            endpoints.end(),
            [&](const TransportEndpoint& endpoint)
            {
                return endpoint.matches(
                    method,
                    path
                );
            }
        );
    }

    bool TransportEndpointCatalog::is_streaming_endpoint(
        TransportMethod method,
        const std::string& path
    )
    {
        const auto endpoints = streaming_endpoints();

        return std::any_of(
            endpoints.begin(),
            endpoints.end(),
            [&](const TransportEndpoint& endpoint)
            {
                return endpoint.matches(
                    method,
                    path
                );
            }
        );
    }
}