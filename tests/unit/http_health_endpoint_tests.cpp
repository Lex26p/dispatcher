#include <dispatcher/http/http_health_endpoint_handlers.hpp>
#include <dispatcher/http/http_request_mapper.hpp>
#include <dispatcher/http/http_response_mapper.hpp>

#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_router_status.hpp>
#include <dispatcher/api/transport_status.hpp>
#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>
#include <dispatcher/runtime/health_status.hpp>
#include <dispatcher/runtime/readiness_status.hpp>

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

    BeastRequest make_get_request(
        std::string target
    )
    {
        return BeastRequest{
            boost::beast::http::verb::get,
            target,
            11
        };
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

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "telemetry",
                "telemetry",
                "telemetry healthy"
            )
        );

        return snapshot;
    }

    dispatcher::runtime::HealthSnapshot make_degraded_snapshot()
    {
        auto snapshot = make_healthy_snapshot();

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::degraded(
                "history",
                "history",
                "history lag"
            )
        );

        return snapshot;
    }

    dispatcher::runtime::HealthSnapshot make_unhealthy_snapshot()
    {
        auto snapshot = make_healthy_snapshot();

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::unhealthy(
                "storage",
                "storage",
                "storage unavailable",
                true
            )
        );

        return snapshot;
    }
}

TEST(HttpHealthEndpointHandlersTests, HealthJsonIncludesAggregateStatus)
{
    const auto snapshot = make_healthy_snapshot();

    const auto json =
        dispatcher::http::HttpHealthEndpointHandlers::health_json(
            snapshot
        );

    EXPECT_NE(json.find("\"status\":\"healthy\""), std::string::npos);
    EXPECT_NE(json.find("\"ready\":true"), std::string::npos);
    EXPECT_NE(json.find("\"check_count\":2"), std::string::npos);
    EXPECT_NE(json.find("\"healthy_count\":2"), std::string::npos);
    EXPECT_NE(json.find("\"degraded_count\":0"), std::string::npos);
    EXPECT_NE(json.find("\"unhealthy_count\":0"), std::string::npos);
    EXPECT_NE(json.find("\"invalid_count\":0"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, ReadinessJsonIncludesReadinessStatus)
{
    const auto snapshot = make_healthy_snapshot();

    const auto json =
        dispatcher::http::HttpHealthEndpointHandlers::readiness_json(
            snapshot
        );

    EXPECT_NE(json.find("\"status\":\"ready\""), std::string::npos);
    EXPECT_NE(json.find("\"ready\":true"), std::string::npos);
    EXPECT_NE(json.find("\"readiness_blockers\":false"), std::string::npos);
    EXPECT_NE(json.find("\"invalid_checks\":false"), std::string::npos);
    EXPECT_NE(json.find("\"check_count\":2"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, HealthyHealthResponseIsHttp200)
{
    const auto response =
        dispatcher::http::HttpHealthEndpointHandlers::health_response(
            make_healthy_snapshot()
        );

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"status\":\"healthy\""), std::string::npos);
    EXPECT_NE(response.body().find("\"ready\":true"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, DegradedHealthResponseIsHttp200)
{
    const auto response =
        dispatcher::http::HttpHealthEndpointHandlers::health_response(
            make_degraded_snapshot()
        );

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"status\":\"degraded\""), std::string::npos);
    EXPECT_NE(response.body().find("\"degraded_count\":1"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, UnhealthyHealthResponseIsHttp200WithUnhealthyJson)
{
    const auto response =
        dispatcher::http::HttpHealthEndpointHandlers::health_response(
            make_unhealthy_snapshot()
        );

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"status\":\"unhealthy\""), std::string::npos);
    EXPECT_NE(response.body().find("\"ready\":false"), std::string::npos);
    EXPECT_NE(response.body().find("\"unhealthy_count\":1"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, ReadyReadinessResponseIsHttp200)
{
    const auto response =
        dispatcher::http::HttpHealthEndpointHandlers::readiness_response(
            make_healthy_snapshot()
        );

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"status\":\"ready\""), std::string::npos);
    EXPECT_NE(response.body().find("\"ready\":true"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, NotReadyReadinessResponseIsHttp200WithNotReadyJson)
{
    const auto response =
        dispatcher::http::HttpHealthEndpointHandlers::readiness_response(
            make_unhealthy_snapshot()
        );

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"status\":\"not_ready\""), std::string::npos);
    EXPECT_NE(response.body().find("\"ready\":false"), std::string::npos);
    EXPECT_NE(response.body().find("\"readiness_blockers\":true"), std::string::npos);
}

TEST(HttpHealthEndpointHandlersTests, RegisterHandlersAddsHealthAndReadyRoutes)
{
    dispatcher::api::TransportRouter router;

    const auto registered =
        dispatcher::http::HttpHealthEndpointHandlers::register_handlers(
            router,
            make_healthy_snapshot()
        );

    ASSERT_TRUE(registered);

    const auto health_request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/health")
        );

    const auto health_result =
        router.dispatch(
            health_request
        );

    ASSERT_TRUE(health_result.ok());

    EXPECT_EQ(
        health_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_EQ(health_result.response().http_status(), 200);
    EXPECT_NE(
        health_result.response().body().find("\"status\":\"healthy\""),
        std::string::npos
    );

    const auto ready_request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/ready")
        );

    const auto ready_result =
        router.dispatch(
            ready_request
        );

    ASSERT_TRUE(ready_result.ok());

    EXPECT_EQ(
        ready_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_EQ(ready_result.response().http_status(), 200);
    EXPECT_NE(
        ready_result.response().body().find("\"status\":\"ready\""),
        std::string::npos
    );
}

TEST(HttpHealthEndpointHandlersTests, BuildRouterReturnsRouterWithHealthRoutes)
{
    auto router =
        dispatcher::http::HttpHealthEndpointHandlers::build_router(
            make_unhealthy_snapshot()
        );

    const auto health_request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/health")
        );

    const auto health_result =
        router.dispatch(
            health_request
        );

    ASSERT_TRUE(health_result.ok());
    EXPECT_EQ(health_result.response().http_status(), 200);
    EXPECT_NE(
        health_result.response().body().find("\"status\":\"unhealthy\""),
        std::string::npos
    );

    const auto ready_request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/ready")
        );

    const auto ready_result =
        router.dispatch(
            ready_request
        );

    ASSERT_TRUE(ready_result.ok());
    EXPECT_EQ(ready_result.response().http_status(), 200);
    EXPECT_NE(
        ready_result.response().body().find("\"status\":\"not_ready\""),
        std::string::npos
    );
}

TEST(HttpHealthEndpointHandlersTests, ResponseCanBeMappedToBeastResponse)
{
    const auto transport_response =
        dispatcher::http::HttpHealthEndpointHandlers::health_response(
            make_healthy_snapshot()
        );

    const auto http_response =
        dispatcher::http::HttpResponseMapper::to_beast_response(
            transport_response
        );

    EXPECT_EQ(http_response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(http_response.result_int(), 200);
    EXPECT_EQ(http_response.body(), transport_response.body());
}

TEST(HttpHealthEndpointHandlersTests, UnhealthyResponseCanBeMappedToBeastResponse)
{
    const auto transport_response =
        dispatcher::http::HttpHealthEndpointHandlers::health_response(
            make_unhealthy_snapshot()
        );

    const auto http_response =
        dispatcher::http::HttpResponseMapper::to_beast_response(
            transport_response
        );

    EXPECT_EQ(http_response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(http_response.result_int(), 200);
    EXPECT_EQ(http_response.body(), transport_response.body());
    EXPECT_NE(http_response.body().find("\"status\":\"unhealthy\""), std::string::npos);
}