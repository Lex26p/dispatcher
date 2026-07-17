#include <dispatcher/http/http_endpoint_router.hpp>
#include <dispatcher/http/http_server.hpp>
#include <dispatcher/http/http_server_options.hpp>

#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace
{
    dispatcher::runtime::HealthSnapshot make_healthy_snapshot()
    {
        dispatcher::runtime::HealthSnapshot snapshot;

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "runtime",
                "runtime",
                "runtime healthy"
            )
        );

        return snapshot;
    }

    dispatcher::api::TransportRouter make_router()
    {
        return dispatcher::http::HttpEndpointRouter::build_router(
            make_healthy_snapshot()
        );
    }
}

TEST(HttpServerTests, StoresOptionsAndRouter)
{
    const auto router =
        make_router();

    dispatcher::http::HttpServer server(
        dispatcher::http::HttpServerOptions(
            "127.0.0.1",
            18080
        ),
        router
    );

    EXPECT_EQ(server.options().bind_address(), "127.0.0.1");
    EXPECT_EQ(server.options().port(), 18080);
    EXPECT_EQ(server.endpoint(), "127.0.0.1:18080");

    EXPECT_TRUE(server.valid());
    EXPECT_TRUE(server.stopped());
    EXPECT_FALSE(server.running());
    EXPECT_FALSE(server.failed());
}

TEST(HttpServerTests, InvalidOptionsAreRejectedBeforeRun)
{
    const auto router =
        make_router();

    dispatcher::http::HttpServer server(
        dispatcher::http::HttpServerOptions(
            "",
            18080
        ),
        router
    );

    EXPECT_FALSE(server.valid());
}

TEST(HttpServerTests, RequestStopSetsStopFlag)
{
    const auto router =
        make_router();

    dispatcher::http::HttpServer server(
        dispatcher::http::HttpServerOptions(
            "127.0.0.1",
            18080
        ),
        router
    );

    EXPECT_FALSE(server.stop_requested());

    server.request_stop();

    EXPECT_TRUE(server.stop_requested());
}

TEST(HttpServerTests, UmbrellaHeaderIncludesHttpServer)
{
    const auto router =
        make_router();

    dispatcher::http::HttpServer server(
        dispatcher::http::HttpServerOptions(
            "127.0.0.1",
            18081
        ),
        router
    );

    EXPECT_TRUE(server.valid());
}