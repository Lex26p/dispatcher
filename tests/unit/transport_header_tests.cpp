#include <dispatcher/api/transport.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>

TEST(TransportHeaderTests, UmbrellaHeaderCanBeIncluded)
{
    dispatcher::api::TransportRouter router;

    const auto add_result =
        router.add_handler(
            dispatcher::api::TransportHandler(
                dispatcher::api::TransportEndpoint(
                    "health check",
                    dispatcher::api::TransportMethod::Get,
                    "/health",
                    false,
                    false,
                    "Health check endpoint"
                ),
                [](const dispatcher::api::TransportRequest&)
                {
                    return dispatcher::api::TransportResponse::success(
                        "{\"status\":\"ok\"}"
                    );
                }
            )
        );

    ASSERT_TRUE(add_result.ok());

    const dispatcher::api::TransportRequest request(
        dispatcher::api::TransportRequestContext(
            dispatcher::api::TransportProtocol::Http,
            "GET",
            "/health",
            "correlation-1",
            {},
            "127.0.0.1"
        )
    );

    const auto result = router.dispatch(request);

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(result.response().http_status(), 200);
    EXPECT_EQ(result.response().body(), "{\"status\":\"ok\"}");

    const auto registry =
        dispatcher::api::TransportEndpointCatalog::build_registry();

    EXPECT_TRUE(
        registry.contains(
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

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportProtocol::Http
        ),
        "http"
    );
}