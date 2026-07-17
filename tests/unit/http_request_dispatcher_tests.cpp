#include <dispatcher/http/http_endpoint_router.hpp>
#include <dispatcher/http/http_request_dispatcher.hpp>

#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{
    using BeastRequest =
        boost::beast::http::request<boost::beast::http::string_body>;

    BeastRequest make_request(
        boost::beast::http::verb method,
        std::string target
    )
    {
        BeastRequest request{
            method,
            target,
            11
        };

        request.set(
            boost::beast::http::field::host,
            "localhost"
        );

        return request;
    }

    BeastRequest make_get_request(
        std::string target
    )
    {
        return make_request(
            boost::beast::http::verb::get,
            std::move(target)
        );
    }

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

TEST(HttpRequestDispatcherTests, DispatchesHealthRequest)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/health"),
            router,
            "127.0.0.1"
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response.result_int(), 200);
    EXPECT_EQ(response.version(), 11);

    EXPECT_NE(response.body().find("\"status\":\"healthy\""), std::string::npos);
    EXPECT_NE(response.body().find("\"ready\":true"), std::string::npos);

    EXPECT_EQ(
        response[boost::beast::http::field::content_type],
        "application/json"
    );

    EXPECT_EQ(
        response[boost::beast::http::field::server],
        "dispatcher-http"
    );
}

TEST(HttpRequestDispatcherTests, DispatchesReadyRequest)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/ready"),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response.result_int(), 200);
    EXPECT_NE(response.body().find("\"status\":\"ready\""), std::string::npos);
}

TEST(HttpRequestDispatcherTests, DispatchesRuntimeRequest)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/api/v1/runtime"),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response.result_int(), 200);
    EXPECT_NE(response.body().find("\"endpoint\":\"runtime\""), std::string::npos);
}

TEST(HttpRequestDispatcherTests, DispatchesAlarmsRequest)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/api/v1/alarms"),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response.result_int(), 200);
    EXPECT_NE(response.body().find("\"endpoint\":\"alarms\""), std::string::npos);
    EXPECT_NE(response.body().find("\"items\":[]"), std::string::npos);
}

TEST(HttpRequestDispatcherTests, DispatchesRuntimeRequestWithQueryString)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/api/v1/runtime?include=summary"),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response.result_int(), 200);
    EXPECT_NE(response.body().find("\"endpoint\":\"runtime\""), std::string::npos);
}

TEST(HttpRequestDispatcherTests, UnknownRouteReturnsNotFound)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_get_request("/missing"),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response.result_int(), 404);
    EXPECT_EQ(response.body(), "{\"status\":\"not_found\"}");

    EXPECT_EQ(
        response[boost::beast::http::field::content_type],
        "application/json"
    );
}

TEST(HttpRequestDispatcherTests, MethodMismatchReturnsNotFound)
{
    const auto router = make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_request(
                boost::beast::http::verb::post,
                "/health"
            ),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response.result_int(), 404);
    EXPECT_EQ(response.body(), "{\"status\":\"not_found\"}");
}

TEST(HttpRequestDispatcherTests, KeepAliveIsPreservedForHandledRoute)
{
    const auto router = make_router();

    auto request = make_get_request(
        "/health"
    );

    request.keep_alive(
        true
    );

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            request,
            router
        );

    EXPECT_TRUE(response.keep_alive());
}

TEST(HttpRequestDispatcherTests, KeepAliveIsPreservedForNotFoundRoute)
{
    const auto router = make_router();

    auto request = make_get_request(
        "/missing"
    );

    request.keep_alive(
        true
    );

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            request,
            router
        );

    EXPECT_TRUE(response.keep_alive());
}

TEST(HttpRequestDispatcherTests, JsonResponseHelperCreatesJsonResponse)
{
    const auto response =
        dispatcher::http::HttpRequestDispatcher::json_response(
            boost::beast::http::status::accepted,
            "{\"status\":\"accepted\"}",
            11,
            false
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::accepted);
    EXPECT_EQ(response.result_int(), 202);
    EXPECT_EQ(response.body(), "{\"status\":\"accepted\"}");
    EXPECT_FALSE(response.keep_alive());

    EXPECT_EQ(
        response[boost::beast::http::field::content_type],
        "application/json"
    );
}

TEST(HttpRequestDispatcherTests, InternalErrorResponseCreatesHttp500)
{
    const auto response =
        dispatcher::http::HttpRequestDispatcher::internal_error_response(
            11,
            false
        );

    EXPECT_EQ(
        response.result(),
        boost::beast::http::status::internal_server_error
    );

    EXPECT_EQ(response.result_int(), 500);
    EXPECT_EQ(response.body(), "{\"status\":\"internal_error\"}");
}