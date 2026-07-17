#include <dispatcher/http/http_endpoint_router.hpp>
#include <dispatcher/http/http_request_dispatcher.hpp>
#include <dispatcher/http/http_response_mapper.hpp>

#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

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
            "Origin",
            "http://localhost:5077"
        );

        return request;
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

    void expect_cors_headers(
        const boost::beast::http::response<boost::beast::http::string_body>& response
    )
    {
        EXPECT_EQ(
            std::string(response["Access-Control-Allow-Origin"]),
            "*"
        );

        EXPECT_NE(
            std::string(response["Access-Control-Allow-Methods"]).find("GET"),
            std::string::npos
        );

        EXPECT_NE(
            std::string(response["Access-Control-Allow-Methods"]).find("OPTIONS"),
            std::string::npos
        );

        EXPECT_NE(
            std::string(response["Access-Control-Allow-Headers"]).find("Content-Type"),
            std::string::npos
        );
    }
}

TEST(HttpCorsTests, TransportResponseMappingAddsCorsHeaders)
{
    const auto transport_response =
        dispatcher::api::TransportResponse::success(
            "{\"status\":\"ok\"}"
        );

    const auto response =
        dispatcher::http::HttpResponseMapper::to_beast_response(
            transport_response
        );

    expect_cors_headers(
        response
    );
}

TEST(HttpCorsTests, HandledGetResponseHasCorsHeaders)
{
    const auto router =
        make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_request(
                boost::beast::http::verb::get,
                "/health"
            ),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::ok);

    expect_cors_headers(
        response
    );
}

TEST(HttpCorsTests, NotFoundResponseHasCorsHeaders)
{
    const auto router =
        make_router();

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            make_request(
                boost::beast::http::verb::get,
                "/missing"
            ),
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::not_found);

    expect_cors_headers(
        response
    );
}

TEST(HttpCorsTests, OptionsPreflightReturnsNoContentWithCorsHeaders)
{
    const auto router =
        make_router();

    auto request =
        make_request(
            boost::beast::http::verb::options,
            "/health"
        );

    request.set(
        "Access-Control-Request-Method",
        "GET"
    );

    const auto response =
        dispatcher::http::HttpRequestDispatcher::dispatch(
            request,
            router
        );

    EXPECT_EQ(response.result(), boost::beast::http::status::no_content);
    EXPECT_EQ(response.result_int(), 204);

    expect_cors_headers(
        response
    );
}

TEST(HttpCorsTests, PreflightHelperDetectsOptionsRequest)
{
    const auto request =
        make_request(
            boost::beast::http::verb::options,
            "/health"
        );

    EXPECT_TRUE(
        dispatcher::http::HttpRequestDispatcher::is_preflight_request(
            request
        )
    );
}

TEST(HttpCorsTests, PreflightHelperRejectsGetRequest)
{
    const auto request =
        make_request(
            boost::beast::http::verb::get,
            "/health"
        );

    EXPECT_FALSE(
        dispatcher::http::HttpRequestDispatcher::is_preflight_request(
            request
        )
    );
}