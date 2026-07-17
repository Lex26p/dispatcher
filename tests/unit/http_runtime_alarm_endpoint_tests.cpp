#include <dispatcher/http/http_alarm_endpoint_handlers.hpp>
#include <dispatcher/http/http_endpoint_router.hpp>
#include <dispatcher/http/http_request_mapper.hpp>
#include <dispatcher/http/http_response_mapper.hpp>
#include <dispatcher/http/http_runtime_endpoint_handlers.hpp>

#include <dispatcher/api/transport_router_status.hpp>
#include <dispatcher/api/transport_status.hpp>
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

        return snapshot;
    }
}

TEST(HttpRuntimeEndpointHandlersTests, RuntimeJsonHasStableContract)
{
    const auto json =
        dispatcher::http::HttpRuntimeEndpointHandlers::runtime_json();

    EXPECT_NE(json.find("\"status\":\"available\""), std::string::npos);
    EXPECT_NE(json.find("\"endpoint\":\"runtime\""), std::string::npos);
    EXPECT_NE(json.find("\"path\":\"/api/v1/runtime\""), std::string::npos);
    EXPECT_NE(json.find("\"method\":\"GET\""), std::string::npos);
    EXPECT_NE(json.find("\"source\":\"dispatcher-http\""), std::string::npos);
}

TEST(HttpRuntimeEndpointHandlersTests, RuntimeResponseIsHttp200)
{
    const auto response =
        dispatcher::http::HttpRuntimeEndpointHandlers::runtime_response();

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"endpoint\":\"runtime\""), std::string::npos);
}

TEST(HttpRuntimeEndpointHandlersTests, RegisterHandlersAddsRuntimeRoute)
{
    dispatcher::api::TransportRouter router;

    const auto registered =
        dispatcher::http::HttpRuntimeEndpointHandlers::register_handlers(
            router
        );

    ASSERT_TRUE(registered);

    const auto request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/api/v1/runtime")
        );

    const auto result =
        router.dispatch(
            request
        );

    ASSERT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_EQ(result.response().http_status(), 200);
    EXPECT_NE(result.response().body().find("\"endpoint\":\"runtime\""), std::string::npos);
}

TEST(HttpAlarmEndpointHandlersTests, AlarmsJsonHasStableContract)
{
    const auto json =
        dispatcher::http::HttpAlarmEndpointHandlers::alarms_json();

    EXPECT_NE(json.find("\"status\":\"available\""), std::string::npos);
    EXPECT_NE(json.find("\"endpoint\":\"alarms\""), std::string::npos);
    EXPECT_NE(json.find("\"path\":\"/api/v1/alarms\""), std::string::npos);
    EXPECT_NE(json.find("\"method\":\"GET\""), std::string::npos);
    EXPECT_NE(json.find("\"source\":\"dispatcher-http\""), std::string::npos);
    EXPECT_NE(json.find("\"items\":[]"), std::string::npos);
}

TEST(HttpAlarmEndpointHandlersTests, AlarmsResponseIsHttp200)
{
    const auto response =
        dispatcher::http::HttpAlarmEndpointHandlers::alarms_response();

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_NE(response.body().find("\"endpoint\":\"alarms\""), std::string::npos);
    EXPECT_NE(response.body().find("\"items\":[]"), std::string::npos);
}

TEST(HttpAlarmEndpointHandlersTests, RegisterHandlersAddsAlarmsRoute)
{
    dispatcher::api::TransportRouter router;

    const auto registered =
        dispatcher::http::HttpAlarmEndpointHandlers::register_handlers(
            router
        );

    ASSERT_TRUE(registered);

    const auto request =
        dispatcher::http::HttpRequestMapper::to_transport_request(
            make_get_request("/api/v1/alarms")
        );

    const auto result =
        router.dispatch(
            request
        );

    ASSERT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_EQ(result.response().http_status(), 200);
    EXPECT_NE(result.response().body().find("\"endpoint\":\"alarms\""), std::string::npos);
}

TEST(HttpEndpointRouterTests, BuildRouterRegistersAllInitialRoutes)
{
    auto router =
        dispatcher::http::HttpEndpointRouter::build_router(
            make_healthy_snapshot()
        );

    const auto health_result =
        router.dispatch(
            dispatcher::http::HttpRequestMapper::to_transport_request(
                make_get_request("/health")
            )
        );

    ASSERT_TRUE(health_result.ok());
    EXPECT_EQ(
        health_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );
    EXPECT_EQ(health_result.response().http_status(), 200);

    const auto ready_result =
        router.dispatch(
            dispatcher::http::HttpRequestMapper::to_transport_request(
                make_get_request("/ready")
            )
        );

    ASSERT_TRUE(ready_result.ok());
    EXPECT_EQ(
        ready_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );
    EXPECT_EQ(ready_result.response().http_status(), 200);

    const auto runtime_result =
        router.dispatch(
            dispatcher::http::HttpRequestMapper::to_transport_request(
                make_get_request("/api/v1/runtime")
            )
        );

    ASSERT_TRUE(runtime_result.ok());
    EXPECT_EQ(
        runtime_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );
    EXPECT_EQ(runtime_result.response().http_status(), 200);

    const auto alarms_result =
        router.dispatch(
            dispatcher::http::HttpRequestMapper::to_transport_request(
                make_get_request("/api/v1/alarms")
            )
        );

    ASSERT_TRUE(alarms_result.ok());
    EXPECT_EQ(
        alarms_result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );
    EXPECT_EQ(alarms_result.response().http_status(), 200);
}

TEST(HttpEndpointRouterTests, RegisterAllReturnsTrue)
{
    dispatcher::api::TransportRouter router;

    const auto registered =
        dispatcher::http::HttpEndpointRouter::register_all(
            router,
            make_healthy_snapshot()
        );

    EXPECT_TRUE(registered);
}

TEST(HttpEndpointRouterTests, RuntimeResponseCanBeMappedToBeastResponse)
{
    const auto transport_response =
        dispatcher::http::HttpRuntimeEndpointHandlers::runtime_response();

    const auto http_response =
        dispatcher::http::HttpResponseMapper::to_beast_response(
            transport_response
        );

    EXPECT_EQ(http_response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(http_response.result_int(), 200);
    EXPECT_EQ(http_response.body(), transport_response.body());
}

TEST(HttpEndpointRouterTests, AlarmsResponseCanBeMappedToBeastResponse)
{
    const auto transport_response =
        dispatcher::http::HttpAlarmEndpointHandlers::alarms_response();

    const auto http_response =
        dispatcher::http::HttpResponseMapper::to_beast_response(
            transport_response
        );

    EXPECT_EQ(http_response.result(), boost::beast::http::status::ok);
    EXPECT_EQ(http_response.result_int(), 200);
    EXPECT_EQ(http_response.body(), transport_response.body());
}