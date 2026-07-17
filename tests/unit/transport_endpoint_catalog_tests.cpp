#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_endpoint_catalog.hpp>
#include <dispatcher/api/transport_endpoint_registry.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_protocol.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

TEST(TransportEndpointCatalogTests, StandardEndpointsAreValid)
{
    const auto endpoints =
        dispatcher::api::TransportEndpointCatalog::standard_endpoints();

    ASSERT_FALSE(endpoints.empty());

    for (const auto& endpoint : endpoints)
    {
        EXPECT_TRUE(endpoint.valid()) << endpoint.key();
        EXPECT_TRUE(endpoint.has_name()) << endpoint.key();
        EXPECT_TRUE(endpoint.has_path()) << endpoint.key();
        EXPECT_TRUE(endpoint.has_description()) << endpoint.key();
    }
}

TEST(TransportEndpointCatalogTests, StandardEndpointsContainCoreRoutes)
{
    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/health"
        )
    );

    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime/stream"
        )
    );

    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/alarms"
        )
    );

    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Post,
            "/api/v1/alarms/acknowledgements"
        )
    );

    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Post,
            "/api/v1/configuration/import"
        )
    );

    EXPECT_FALSE(
        dispatcher::api::TransportEndpointCatalog::is_standard_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/missing"
        )
    );
}

TEST(TransportEndpointCatalogTests, HealthEndpointIsPublic)
{
    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_public_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/health"
        )
    );

    EXPECT_FALSE(
        dispatcher::api::TransportEndpointCatalog::is_public_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    const auto public_endpoints =
        dispatcher::api::TransportEndpointCatalog::public_endpoints();

    ASSERT_EQ(public_endpoints.size(), 1);

    EXPECT_EQ(public_endpoints.front().key(), "GET /health");
    EXPECT_TRUE(public_endpoints.front().public_endpoint());
}

TEST(TransportEndpointCatalogTests, AuthenticatedEndpointsExcludeHealth)
{
    const auto authenticated_endpoints =
        dispatcher::api::TransportEndpointCatalog::authenticated_endpoints();

    ASSERT_FALSE(authenticated_endpoints.empty());

    for (const auto& endpoint : authenticated_endpoints)
    {
        EXPECT_TRUE(endpoint.requires_authentication()) << endpoint.key();
        EXPECT_NE(endpoint.key(), "GET /health");
    }

    EXPECT_TRUE(
        authenticated_endpoints.size()
        <
        dispatcher::api::TransportEndpointCatalog::standard_endpoints().size()
    );
}

TEST(TransportEndpointCatalogTests, RuntimeStreamIsStreamingOnly)
{
    EXPECT_TRUE(
        dispatcher::api::TransportEndpointCatalog::is_streaming_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime/stream"
        )
    );

    EXPECT_FALSE(
        dispatcher::api::TransportEndpointCatalog::is_streaming_endpoint(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    const auto streaming_endpoints =
        dispatcher::api::TransportEndpointCatalog::streaming_endpoints();

    ASSERT_EQ(streaming_endpoints.size(), 1);

    EXPECT_EQ(
        streaming_endpoints.front().key(),
        "GET /api/v1/runtime/stream"
    );

    EXPECT_TRUE(streaming_endpoints.front().streaming());

    EXPECT_FALSE(
        streaming_endpoints.front().compatible_with(
            dispatcher::api::TransportProtocol::Http
        )
    );

    EXPECT_TRUE(
        streaming_endpoints.front().compatible_with(
            dispatcher::api::TransportProtocol::Grpc
        )
    );
}

TEST(TransportEndpointCatalogTests, BuildRegistryRegistersAllEndpoints)
{
    const auto endpoints =
        dispatcher::api::TransportEndpointCatalog::standard_endpoints();

    const auto registry =
        dispatcher::api::TransportEndpointCatalog::build_registry();

    EXPECT_EQ(registry.size(), endpoints.size());

    EXPECT_TRUE(
        registry.contains(
            dispatcher::api::TransportMethod::Get,
            "/health"
        )
    );

    EXPECT_TRUE(
        registry.contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_TRUE(
        registry.contains(
            dispatcher::api::TransportMethod::Post,
            "/api/v1/notifications/routes"
        )
    );
}

TEST(TransportEndpointCatalogTests, RegisterStandardEndpointsReturnsResults)
{
    dispatcher::api::TransportEndpointRegistry registry;

    const auto results =
        dispatcher::api::TransportEndpointCatalog::register_standard_endpoints(
            registry
        );

    const auto endpoints =
        dispatcher::api::TransportEndpointCatalog::standard_endpoints();

    EXPECT_EQ(results.size(), endpoints.size());
    EXPECT_EQ(registry.size(), endpoints.size());

    for (const auto& result : results)
    {
        EXPECT_TRUE(result.ok());
        EXPECT_TRUE(result.has_endpoint());
    }
}

TEST(TransportEndpointCatalogTests, RegisterStandardEndpointsRejectsDuplicates)
{
    dispatcher::api::TransportEndpointRegistry registry;

    const auto first_results =
        dispatcher::api::TransportEndpointCatalog::register_standard_endpoints(
            registry
        );

    const auto second_results =
        dispatcher::api::TransportEndpointCatalog::register_standard_endpoints(
            registry
        );

    ASSERT_FALSE(first_results.empty());
    ASSERT_FALSE(second_results.empty());

    for (const auto& result : first_results)
    {
        EXPECT_TRUE(result.ok());
    }

    for (const auto& result : second_results)
    {
        EXPECT_TRUE(result.failed());
    }

    EXPECT_EQ(
        registry.size(),
        dispatcher::api::TransportEndpointCatalog::standard_endpoints().size()
    );
}

TEST(TransportEndpointCatalogTests, CatalogDoesNotContainDuplicateKeys)
{
    const auto endpoints =
        dispatcher::api::TransportEndpointCatalog::standard_endpoints();

    std::vector<std::string> keys;

    keys.reserve(endpoints.size());

    for (const auto& endpoint : endpoints)
    {
        keys.push_back(endpoint.key());
    }

    std::sort(
        keys.begin(),
        keys.end()
    );

    const auto duplicate =
        std::adjacent_find(
            keys.begin(),
            keys.end()
        );

    EXPECT_EQ(duplicate, keys.end());
}