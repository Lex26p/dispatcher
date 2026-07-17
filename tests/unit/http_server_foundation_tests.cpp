#include <dispatcher/http/http.hpp>

#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/api/transport_status.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <gtest/gtest.h>

TEST(HttpServerStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Stopped
        ),
        "stopped"
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Starting
        ),
        "starting"
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Running
        ),
        "running"
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Stopping
        ),
        "stopping"
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(
            dispatcher::http::HttpServerStatus::Failed
        ),
        "failed"
    );
}

TEST(HttpServerStatusTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::http::is_known(
            dispatcher::http::HttpServerStatus::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_stopped(
            dispatcher::http::HttpServerStatus::Stopped
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_starting(
            dispatcher::http::HttpServerStatus::Starting
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_running(
            dispatcher::http::HttpServerStatus::Running
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_stopping(
            dispatcher::http::HttpServerStatus::Stopping
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_failed(
            dispatcher::http::HttpServerStatus::Failed
        )
    );

    EXPECT_TRUE(
        dispatcher::http::is_terminal(
            dispatcher::http::HttpServerStatus::Stopped
        )
    );

    EXPECT_TRUE(
        dispatcher::http::accepts_start(
            dispatcher::http::HttpServerStatus::Stopped
        )
    );

    EXPECT_TRUE(
        dispatcher::http::accepts_stop(
            dispatcher::http::HttpServerStatus::Running
        )
    );

    EXPECT_FALSE(
        dispatcher::http::accepts_start(
            dispatcher::http::HttpServerStatus::Running
        )
    );
}

TEST(HttpServerOptionsTests, DefaultOptionsAreValid)
{
    const dispatcher::http::HttpServerOptions options;

    EXPECT_EQ(options.bind_address(), "127.0.0.1");
    EXPECT_EQ(options.port(), 8080);
    EXPECT_EQ(options.worker_threads(), 1);
    EXPECT_EQ(options.request_body_limit_bytes(), 1024ULL * 1024ULL);
    EXPECT_TRUE(options.reuse_address());

    EXPECT_TRUE(options.has_bind_address());
    EXPECT_TRUE(options.has_valid_port());
    EXPECT_TRUE(options.has_worker_threads());
    EXPECT_TRUE(options.has_request_body_limit());
    EXPECT_TRUE(options.valid());

    EXPECT_EQ(options.endpoint(), "127.0.0.1:8080");
}

TEST(HttpServerOptionsTests, CustomOptionsAreStored)
{
    const dispatcher::http::HttpServerOptions options(
        "0.0.0.0",
        9000,
        4,
        2ULL * 1024ULL * 1024ULL,
        false
    );

    EXPECT_EQ(options.bind_address(), "0.0.0.0");
    EXPECT_EQ(options.port(), 9000);
    EXPECT_EQ(options.worker_threads(), 4);
    EXPECT_EQ(options.request_body_limit_bytes(), 2ULL * 1024ULL * 1024ULL);
    EXPECT_FALSE(options.reuse_address());
    EXPECT_TRUE(options.valid());

    EXPECT_EQ(options.endpoint(), "0.0.0.0:9000");
}

TEST(HttpServerOptionsTests, InvalidOptionsAreRejected)
{
    const dispatcher::http::HttpServerOptions missing_address(
        "",
        8080
    );

    EXPECT_FALSE(missing_address.valid());

    const dispatcher::http::HttpServerOptions missing_port(
        "127.0.0.1",
        0
    );

    EXPECT_FALSE(missing_port.valid());

    const dispatcher::http::HttpServerOptions missing_workers(
        "127.0.0.1",
        8080,
        0
    );

    EXPECT_FALSE(missing_workers.valid());

    const dispatcher::http::HttpServerOptions missing_body_limit(
        "127.0.0.1",
        8080,
        1,
        0
    );

    EXPECT_FALSE(missing_body_limit.valid());
}

TEST(HttpServerResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::http::HttpServerResult::success(
            dispatcher::http::HttpServerStatus::Running,
            "started"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::http::HttpServerStatus::Running
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "started");
}

TEST(HttpServerResultTests, FailureResultWorks)
{
    const auto result =
        dispatcher::http::HttpServerResult::failure(
            dispatcher::http::HttpServerStatus::Failed,
            "failed"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::http::HttpServerStatus::Failed
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "failed");
}

TEST(HttpServerAdapterTests, StartsAndStopsWithValidRouter)
{
    dispatcher::api::TransportRouter router;

    dispatcher::http::HttpServerAdapter server(
        dispatcher::http::HttpServerOptions{},
        &router
    );

    EXPECT_TRUE(server.has_router());
    EXPECT_TRUE(server.stopped());
    EXPECT_TRUE(server.can_start());

    const auto start_result = server.start();

    ASSERT_TRUE(start_result.ok());

    EXPECT_TRUE(server.running());

    EXPECT_EQ(
        server.status(),
        dispatcher::http::HttpServerStatus::Running
    );

    const auto stop_result = server.stop();

    ASSERT_TRUE(stop_result.ok());

    EXPECT_TRUE(server.stopped());

    EXPECT_EQ(
        server.status(),
        dispatcher::http::HttpServerStatus::Stopped
    );
}

TEST(HttpServerAdapterTests, StartFailsWithoutRouter)
{
    dispatcher::http::HttpServerAdapter server;

    EXPECT_FALSE(server.has_router());

    const auto result = server.start();

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        server.status(),
        dispatcher::http::HttpServerStatus::Failed
    );

    EXPECT_TRUE(result.has_message());
}

TEST(HttpServerAdapterTests, StartFailsWithInvalidOptions)
{
    dispatcher::api::TransportRouter router;

    dispatcher::http::HttpServerAdapter server(
        dispatcher::http::HttpServerOptions(
            "",
            8080
        ),
        &router
    );

    const auto result = server.start();

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        server.status(),
        dispatcher::http::HttpServerStatus::Failed
    );
}

TEST(HttpServerAdapterTests, RouterCanBeAssignedLater)
{
    dispatcher::api::TransportRouter router;

    dispatcher::http::HttpServerAdapter server;

    EXPECT_FALSE(server.has_router());

    server.set_router(&router);

    EXPECT_TRUE(server.has_router());
    EXPECT_EQ(server.router(), &router);

    const auto result = server.start();

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(server.running());
}

TEST(HttpServerAdapterTests, MarkFailedChangesStatus)
{
    dispatcher::http::HttpServerAdapter server;

    const auto result = server.mark_failed(
        "manual failure"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(server.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::http::HttpServerStatus::Failed
    );

    EXPECT_EQ(result.message(), "manual failure");
}

TEST(HttpServerFoundationTests, BoostBeastAndAsioHeadersAreAvailable)
{
    boost::asio::io_context io_context;
    boost::beast::flat_buffer buffer;

    EXPECT_FALSE(io_context.stopped());
    EXPECT_EQ(buffer.size(), 0);
}

TEST(HttpServerFoundationTests, UmbrellaHeaderCanBeIncluded)
{
    dispatcher::api::TransportRouter router;

    dispatcher::http::HttpServerAdapter server(
        dispatcher::http::HttpServerOptions{},
        &router
    );

    const auto result = server.start();

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        server.status(),
        dispatcher::http::HttpServerStatus::Running
    );

    EXPECT_STREQ(
        dispatcher::http::to_string(server.status()),
        "running"
    );
}