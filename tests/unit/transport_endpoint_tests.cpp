#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_endpoint_registry.hpp>
#include <dispatcher/api/transport_endpoint_result.hpp>
#include <dispatcher/api/transport_endpoint_status.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_protocol.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace
{
    dispatcher::api::TransportEndpoint make_endpoint(
        std::string name = "runtime snapshot",
        dispatcher::api::TransportMethod method =
        dispatcher::api::TransportMethod::Get,
        std::string path = "/api/v1/runtime",
        bool requires_authentication = true,
        bool streaming = false,
        std::string description = "runtime snapshot endpoint"
    )
    {
        return dispatcher::api::TransportEndpoint(
            std::move(name),
            method,
            std::move(path),
            requires_authentication,
            streaming,
            std::move(description)
        );
    }
}

TEST(TransportMethodTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Get
        ),
        "GET"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Post
        ),
        "POST"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Put
        ),
        "PUT"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Patch
        ),
        "PATCH"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportMethod::Delete
        ),
        "DELETE"
    );
}

TEST(TransportMethodTests, ParseFromStringWorks)
{
    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("GET"),
        dispatcher::api::TransportMethod::Get
    );

    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("post"),
        dispatcher::api::TransportMethod::Post
    );

    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("PUT"),
        dispatcher::api::TransportMethod::Put
    );

    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("patch"),
        dispatcher::api::TransportMethod::Patch
    );

    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("DELETE"),
        dispatcher::api::TransportMethod::Delete
    );

    EXPECT_EQ(
        dispatcher::api::transport_method_from_string("OPTIONS"),
        dispatcher::api::TransportMethod::Unknown
    );
}

TEST(TransportMethodTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::api::is_known_method(
            dispatcher::api::TransportMethod::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_known_method(
            dispatcher::api::TransportMethod::Get
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_read_method(
            dispatcher::api::TransportMethod::Get
        )
    );

    EXPECT_FALSE(
        dispatcher::api::is_read_method(
            dispatcher::api::TransportMethod::Post
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_write_method(
            dispatcher::api::TransportMethod::Post
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_write_method(
            dispatcher::api::TransportMethod::Delete
        )
    );

    EXPECT_FALSE(
        dispatcher::api::is_write_method(
            dispatcher::api::TransportMethod::Get
        )
    );
}

TEST(TransportEndpointStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::Registered
        ),
        "registered"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::Removed
        ),
        "removed"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::Found
        ),
        "found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::DuplicateEndpoint
        ),
        "duplicate_endpoint"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportEndpointStatus::InvalidEndpoint
        ),
        "invalid_endpoint"
    );
}

TEST(TransportEndpointStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportEndpointStatus::Registered
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportEndpointStatus::Found
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportEndpointStatus::NotFound
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportEndpointStatus::InvalidEndpoint
        )
    );
}

TEST(TransportEndpointTests, EndpointCapturesFields)
{
    const auto endpoint = make_endpoint();

    EXPECT_EQ(endpoint.name(), "runtime snapshot");

    EXPECT_EQ(
        endpoint.method(),
        dispatcher::api::TransportMethod::Get
    );

    EXPECT_EQ(endpoint.path(), "/api/v1/runtime");

    EXPECT_TRUE(endpoint.requires_authentication());
    EXPECT_FALSE(endpoint.public_endpoint());

    EXPECT_FALSE(endpoint.streaming());

    EXPECT_EQ(endpoint.description(), "runtime snapshot endpoint");

    EXPECT_TRUE(endpoint.has_name());
    EXPECT_TRUE(endpoint.has_path());
    EXPECT_TRUE(endpoint.has_description());

    EXPECT_TRUE(endpoint.valid());

    EXPECT_EQ(endpoint.key(), "GET /api/v1/runtime");
}

TEST(TransportEndpointTests, PublicEndpointWorks)
{
    const auto endpoint = make_endpoint(
        "health",
        dispatcher::api::TransportMethod::Get,
        "/health",
        false
    );

    EXPECT_FALSE(endpoint.requires_authentication());
    EXPECT_TRUE(endpoint.public_endpoint());
    EXPECT_TRUE(endpoint.valid());
}

TEST(TransportEndpointTests, EndpointRequiresNameKnownMethodAndAbsolutePath)
{
    EXPECT_FALSE(
        make_endpoint(
            "",
            dispatcher::api::TransportMethod::Get,
            "/api"
        ).valid()
    );

    EXPECT_FALSE(
        make_endpoint(
            "bad method",
            dispatcher::api::TransportMethod::Unknown,
            "/api"
        ).valid()
    );

    EXPECT_FALSE(
        make_endpoint(
            "missing path",
            dispatcher::api::TransportMethod::Get,
            ""
        ).valid()
    );

    EXPECT_FALSE(
        make_endpoint(
            "relative path",
            dispatcher::api::TransportMethod::Get,
            "api/v1/runtime"
        ).valid()
    );
}

TEST(TransportEndpointTests, MatchesMethodAndPath)
{
    const auto endpoint = make_endpoint();

    EXPECT_TRUE(
        endpoint.matches(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_FALSE(
        endpoint.matches(
            dispatcher::api::TransportMethod::Post,
            "/api/v1/runtime"
        )
    );

    EXPECT_FALSE(
        endpoint.matches(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/configuration"
        )
    );
}

TEST(TransportEndpointTests, StreamingEndpointRequiresStreamingProtocol)
{
    const auto endpoint = make_endpoint(
        "runtime stream",
        dispatcher::api::TransportMethod::Get,
        "/api/v1/runtime/stream",
        true,
        true
    );

    EXPECT_TRUE(endpoint.streaming());

    EXPECT_FALSE(
        endpoint.compatible_with(
            dispatcher::api::TransportProtocol::Http
        )
    );

    EXPECT_TRUE(
        endpoint.compatible_with(
            dispatcher::api::TransportProtocol::Grpc
        )
    );

    EXPECT_FALSE(
        endpoint.compatible_with(
            dispatcher::api::TransportProtocol::Unknown
        )
    );
}

TEST(TransportEndpointResultTests, SuccessResultContainsEndpoint)
{
    const auto result =
        dispatcher::api::TransportEndpointResult::success(
            dispatcher::api::TransportEndpointStatus::Registered,
            make_endpoint(),
            "endpoint registered"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::Registered
    );

    EXPECT_TRUE(result.has_endpoint());
    EXPECT_EQ(result.endpoint().name(), "runtime snapshot");

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "endpoint registered");

    EXPECT_FALSE(result.has_field());
    EXPECT_FALSE(result.has_value());
}

TEST(TransportEndpointResultTests, FailureResultDoesNotContainEndpoint)
{
    const auto result =
        dispatcher::api::TransportEndpointResult::failure(
            dispatcher::api::TransportEndpointStatus::NotFound,
            "endpoint not found",
            "endpoint",
            "GET /missing"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::NotFound
    );

    EXPECT_FALSE(result.has_endpoint());

    EXPECT_TRUE(result.has_message());
    EXPECT_TRUE(result.has_field());
    EXPECT_TRUE(result.has_value());

    EXPECT_THROW(
        (void)result.endpoint(),
        std::logic_error
    );
}

TEST(TransportEndpointRegistryTests, DefaultRegistryIsEmpty)
{
    const dispatcher::api::TransportEndpointRegistry registry;

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);

    EXPECT_FALSE(
        registry.contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_TRUE(registry.endpoints().empty());
}

TEST(TransportEndpointRegistryTests, AddStoresEndpoint)
{
    dispatcher::api::TransportEndpointRegistry registry;

    const auto result =
        registry.add(
            make_endpoint()
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::Registered
    );

    EXPECT_FALSE(registry.empty());
    EXPECT_EQ(registry.size(), 1);

    EXPECT_TRUE(
        registry.contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    const auto found =
        registry.find(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        );

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name(), "runtime snapshot");
}

TEST(TransportEndpointRegistryTests, AddRejectsInvalidEndpoint)
{
    dispatcher::api::TransportEndpointRegistry registry;

    const auto result =
        registry.add(
            make_endpoint(
                "",
                dispatcher::api::TransportMethod::Get,
                "/api"
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::InvalidEndpoint
    );

    EXPECT_EQ(result.field(), "endpoint");
    EXPECT_TRUE(registry.empty());
}

TEST(TransportEndpointRegistryTests, AddRejectsDuplicateEndpoint)
{
    dispatcher::api::TransportEndpointRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_endpoint()
        ).ok()
    );

    const auto result =
        registry.add(
            make_endpoint(
                "runtime snapshot duplicate",
                dispatcher::api::TransportMethod::Get,
                "/api/v1/runtime"
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::DuplicateEndpoint
    );

    EXPECT_EQ(result.field(), "endpoint");
    EXPECT_EQ(result.value(), "GET /api/v1/runtime");
    EXPECT_EQ(registry.size(), 1);
}

TEST(TransportEndpointRegistryTests, ResolveReturnsFoundEndpoint)
{
    dispatcher::api::TransportEndpointRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_endpoint()
        ).ok()
    );

    const auto result =
        registry.resolve(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::Found
    );

    EXPECT_EQ(result.endpoint().name(), "runtime snapshot");
}

TEST(TransportEndpointRegistryTests, ResolveMissingEndpointFails)
{
    const dispatcher::api::TransportEndpointRegistry registry;

    const auto result =
        registry.resolve(
            dispatcher::api::TransportMethod::Get,
            "/missing"
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::NotFound
    );

    EXPECT_EQ(result.field(), "endpoint");
    EXPECT_EQ(result.value(), "GET /missing");
}

TEST(TransportEndpointRegistryTests, RemoveDeletesEndpoint)
{
    dispatcher::api::TransportEndpointRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_endpoint()
        ).ok()
    );

    const auto result =
        registry.remove(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::Removed
    );

    EXPECT_TRUE(registry.empty());

    EXPECT_FALSE(
        registry.contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );
}

TEST(TransportEndpointRegistryTests, RemoveMissingEndpointFails)
{
    dispatcher::api::TransportEndpointRegistry registry;

    const auto result =
        registry.remove(
            dispatcher::api::TransportMethod::Get,
            "/missing"
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::NotFound
    );
}

TEST(TransportEndpointRegistryTests, EndpointsAreReturnedSortedByPathThenMethod)
{
    dispatcher::api::TransportEndpointRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_endpoint(
                "post config",
                dispatcher::api::TransportMethod::Post,
                "/api/v1/configuration"
            )
        ).ok()
    );

    ASSERT_TRUE(
        registry.add(
            make_endpoint(
                "get runtime",
                dispatcher::api::TransportMethod::Get,
                "/api/v1/runtime"
            )
        ).ok()
    );

    ASSERT_TRUE(
        registry.add(
            make_endpoint(
                "get config",
                dispatcher::api::TransportMethod::Get,
                "/api/v1/configuration"
            )
        ).ok()
    );

    const auto endpoints = registry.endpoints();

    ASSERT_EQ(endpoints.size(), 3);

    EXPECT_EQ(endpoints[0].key(), "GET /api/v1/configuration");
    EXPECT_EQ(endpoints[1].key(), "POST /api/v1/configuration");
    EXPECT_EQ(endpoints[2].key(), "GET /api/v1/runtime");
}

TEST(TransportEndpointRegistryTests, ClearRemovesEverything)
{
    dispatcher::api::TransportEndpointRegistry registry;

    ASSERT_TRUE(
        registry.add(
            make_endpoint()
        ).ok()
    );

    ASSERT_EQ(registry.size(), 1);

    registry.clear();

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);
    EXPECT_TRUE(registry.endpoints().empty());
}