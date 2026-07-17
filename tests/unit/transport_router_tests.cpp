#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_error.hpp>
#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_request_context.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/api/transport_router_result.hpp>
#include <dispatcher/api/transport_router_status.hpp>
#include <dispatcher/api/transport_status.hpp>

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    dispatcher::api::TransportRequest make_router_request(
        std::string method = "GET",
        std::string path = "/api/v1/runtime",
        dispatcher::api::TransportProtocol protocol =
        dispatcher::api::TransportProtocol::Http,
        std::string body = {}
    )
    {
        return dispatcher::api::TransportRequest(
            dispatcher::api::TransportRequestContext(
                protocol,
                std::move(method),
                std::move(path),
                "correlation-1",
                "operator-1",
                "127.0.0.1"
            ),
            std::move(body),
            "application/json"
        );
    }

    dispatcher::api::TransportEndpoint make_router_endpoint(
        std::string name = "runtime snapshot",
        dispatcher::api::TransportMethod method =
        dispatcher::api::TransportMethod::Get,
        std::string path = "/api/v1/runtime",
        bool streaming = false
    )
    {
        return dispatcher::api::TransportEndpoint(
            std::move(name),
            method,
            std::move(path),
            true,
            streaming,
            "test endpoint"
        );
    }

    dispatcher::api::TransportHandler make_router_handler(
        dispatcher::api::TransportEndpoint endpoint =
        make_router_endpoint(),
        bool enabled = true
    )
    {
        return dispatcher::api::TransportHandler(
            std::move(endpoint),
            [](const dispatcher::api::TransportRequest& request)
            {
                return dispatcher::api::TransportResponse::success(
                    std::string{ "{\"path\":\"" } + request.path() + "\"}"
                );
            },
            enabled
        );
    }
}

TEST(TransportRequestTests, RequestCapturesContextAndBody)
{
    const auto request =
        make_router_request(
            "POST",
            "/api/v1/configuration",
            dispatcher::api::TransportProtocol::Http,
            "{\"import\":true}"
        );

    EXPECT_EQ(
        request.protocol(),
        dispatcher::api::TransportProtocol::Http
    );

    EXPECT_EQ(
        request.method(),
        dispatcher::api::TransportMethod::Post
    );

    EXPECT_EQ(request.method_text(), "POST");
    EXPECT_EQ(request.path(), "/api/v1/configuration");

    EXPECT_TRUE(request.has_body());
    EXPECT_EQ(request.body(), "{\"import\":true}");

    EXPECT_TRUE(request.has_content_type());
    EXPECT_EQ(request.content_type(), "application/json");

    EXPECT_TRUE(request.valid());
}

TEST(TransportRequestTests, RequestIsInvalidWhenMethodIsUnknown)
{
    const auto request =
        make_router_request(
            "OPTIONS",
            "/api/v1/runtime"
        );

    EXPECT_EQ(
        request.method(),
        dispatcher::api::TransportMethod::Unknown
    );

    EXPECT_FALSE(request.valid());
}

TEST(TransportRouterStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportRouterStatus::Handled
        ),
        "handled"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportRouterStatus::InvalidRequest
        ),
        "invalid_request"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportRouterStatus::EndpointNotFound
        ),
        "endpoint_not_found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportRouterStatus::HandlerNotFound
        ),
        "handler_not_found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportRouterStatus::HandlerFailed
        ),
        "handler_failed"
    );
}

TEST(TransportRouterStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportRouterStatus::Handled
        )
    );

    EXPECT_FALSE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportRouterStatus::Handled
        )
    );

    EXPECT_FALSE(
        dispatcher::api::is_success(
            dispatcher::api::TransportRouterStatus::EndpointNotFound
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportRouterStatus::HandlerFailed
        )
    );
}

TEST(TransportHandlerTests, HandlerCapturesEndpointAndHandlesRequest)
{
    const auto handler =
        make_router_handler();

    EXPECT_TRUE(handler.enabled());
    EXPECT_FALSE(handler.disabled());

    EXPECT_TRUE(handler.has_handler());
    EXPECT_TRUE(handler.valid());

    EXPECT_TRUE(
        handler.matches(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_TRUE(
        handler.compatible_with(
            dispatcher::api::TransportProtocol::Http
        )
    );

    const auto response =
        handler.handle(
            make_router_request()
        );

    EXPECT_TRUE(response.ok());
    EXPECT_EQ(response.body(), "{\"path\":\"/api/v1/runtime\"}");
}

TEST(TransportHandlerTests, DisabledHandlerReturnsUnavailable)
{
    const auto handler =
        make_router_handler(
            make_router_endpoint(),
            false
        );

    EXPECT_FALSE(handler.enabled());
    EXPECT_TRUE(handler.disabled());

    const auto response =
        handler.handle(
            make_router_request()
        );

    EXPECT_TRUE(response.failed());

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Unavailable
    );

    ASSERT_TRUE(response.error().has_value());
    EXPECT_EQ(response.error()->code(), "internal_error");
}

TEST(TransportHandlerTests, InvalidHandlerReturnsInternalError)
{
    const dispatcher::api::TransportHandler handler(
        make_router_endpoint(),
        {}
    );

    EXPECT_FALSE(handler.has_handler());
    EXPECT_FALSE(handler.valid());

    const auto response =
        handler.handle(
            make_router_request()
        );

    EXPECT_TRUE(response.failed());

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::InternalError
    );
}

TEST(TransportRouterResultTests, HandledResultContainsResponseAndEndpoint)
{
    const auto result =
        dispatcher::api::TransportRouterResult::handled(
            dispatcher::api::TransportResponse::success("{}"),
            make_router_endpoint(),
            "handled"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_TRUE(result.has_response());
    EXPECT_TRUE(result.has_endpoint());

    EXPECT_EQ(result.response().http_status(), 200);
    EXPECT_EQ(result.endpoint().key(), "GET /api/v1/runtime");

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "handled");

    EXPECT_FALSE(result.has_reason());
}

TEST(TransportRouterResultTests, FailedResultContainsResponseAndNoEndpoint)
{
    const auto result =
        dispatcher::api::TransportRouterResult::failed(
            dispatcher::api::TransportRouterStatus::EndpointNotFound,
            dispatcher::api::TransportResponse::failure(
                dispatcher::api::TransportStatus::NotFound,
                dispatcher::api::TransportError::not_found(
                    "endpoint not found"
                )
            ),
            "endpoint not found",
            "endpoint",
            "GET /missing"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::EndpointNotFound
    );

    EXPECT_TRUE(result.has_response());
    EXPECT_FALSE(result.has_endpoint());

    EXPECT_EQ(result.response().http_status(), 404);

    EXPECT_TRUE(result.has_reason());
    EXPECT_TRUE(result.has_field());
    EXPECT_TRUE(result.has_value());

    EXPECT_THROW(
        (void)result.endpoint(),
        std::logic_error
    );
}

TEST(TransportRouterTests, DefaultRouterIsEmpty)
{
    const dispatcher::api::TransportRouter router;

    EXPECT_TRUE(router.empty());
    EXPECT_EQ(router.handler_count(), 0);
    EXPECT_TRUE(router.endpoints().empty());

    EXPECT_FALSE(
        router.contains_handler(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );
}

TEST(TransportRouterTests, AddHandlerRegistersEndpointAndHandler)
{
    dispatcher::api::TransportRouter router;

    const auto result =
        router.add_handler(
            make_router_handler()
        );

    EXPECT_TRUE(result.ok());

    EXPECT_FALSE(router.empty());
    EXPECT_EQ(router.handler_count(), 1);

    EXPECT_TRUE(
        router.contains_handler(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_TRUE(
        router.endpoint_registry().contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    const auto handler =
        router.find_handler(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        );

    ASSERT_TRUE(handler.has_value());
    EXPECT_TRUE(handler->valid());
}

TEST(TransportRouterTests, AddHandlerRejectsInvalidHandler)
{
    dispatcher::api::TransportRouter router;

    const auto result =
        router.add_handler(
            dispatcher::api::TransportHandler(
                make_router_endpoint(),
                {}
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::InvalidEndpoint
    );

    EXPECT_TRUE(router.empty());
}

TEST(TransportRouterTests, AddHandlerRejectsDuplicateEndpoint)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler()
        ).ok()
    );

    const auto result =
        router.add_handler(
            make_router_handler()
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportEndpointStatus::DuplicateEndpoint
    );

    EXPECT_EQ(router.handler_count(), 1);
}

TEST(TransportRouterTests, RemoveHandlerDeletesEndpointAndHandler)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler()
        ).ok()
    );

    const auto result =
        router.remove_handler(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        );

    EXPECT_TRUE(result.ok());

    EXPECT_TRUE(router.empty());
    EXPECT_EQ(router.handler_count(), 0);

    EXPECT_FALSE(
        router.contains_handler(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );

    EXPECT_FALSE(
        router.endpoint_registry().contains(
            dispatcher::api::TransportMethod::Get,
            "/api/v1/runtime"
        )
    );
}

TEST(TransportRouterTests, DispatchInvalidRequestReturnsBadRequest)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler()
        ).ok()
    );

    const auto result =
        router.dispatch(
            make_router_request(
                "OPTIONS",
                "/api/v1/runtime"
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::InvalidRequest
    );

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::BadRequest
    );

    ASSERT_TRUE(result.response().error().has_value());
    EXPECT_EQ(result.response().error()->code(), "invalid_request");
}

TEST(TransportRouterTests, DispatchMissingEndpointReturnsNotFound)
{
    dispatcher::api::TransportRouter router;

    const auto result =
        router.dispatch(
            make_router_request()
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::EndpointNotFound
    );

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::NotFound
    );

    EXPECT_EQ(result.field(), "endpoint");
    EXPECT_EQ(result.value(), "GET /api/v1/runtime");
}

TEST(TransportRouterTests, DispatchRejectsIncompatibleStreamingEndpoint)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler(
                make_router_endpoint(
                    "runtime stream",
                    dispatcher::api::TransportMethod::Get,
                    "/api/v1/runtime/stream",
                    true
                )
            )
        ).ok()
    );

    const auto result =
        router.dispatch(
            make_router_request(
                "GET",
                "/api/v1/runtime/stream",
                dispatcher::api::TransportProtocol::Http
            )
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::InvalidRequest
    );

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::BadRequest
    );

    EXPECT_EQ(result.field(), "protocol");
    EXPECT_EQ(result.value(), "http");
}

TEST(TransportRouterTests, DispatchInvokesMatchingHandler)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler()
        ).ok()
    );

    const auto result =
        router.dispatch(
            make_router_request()
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_TRUE(result.has_endpoint());

    EXPECT_EQ(
        result.endpoint().key(),
        "GET /api/v1/runtime"
    );

    EXPECT_TRUE(result.response().ok());
    EXPECT_EQ(result.response().body(), "{\"path\":\"/api/v1/runtime\"}");
}

TEST(TransportRouterTests, DispatchTreatsHandlerFailureResponseAsHandled)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            dispatcher::api::TransportHandler(
                make_router_endpoint(),
                [](const dispatcher::api::TransportRequest&)
                {
                    return dispatcher::api::TransportResponse::failure(
                        dispatcher::api::TransportStatus::BadRequest,
                        dispatcher::api::TransportError::invalid_request(
                            "bad request"
                        )
                    );
                }
            )
        ).ok()
    );

    const auto result =
        router.dispatch(
            make_router_request()
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::Handled
    );

    EXPECT_TRUE(result.response().failed());

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::BadRequest
    );
}

TEST(TransportRouterTests, DispatchCatchesHandlerException)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            dispatcher::api::TransportHandler(
                make_router_endpoint(),
                [](const dispatcher::api::TransportRequest&)
                -> dispatcher::api::TransportResponse
                {
                    throw std::runtime_error("handler exploded");
                }
            )
        ).ok()
    );

    const auto result =
        router.dispatch(
            make_router_request()
        );

    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::TransportRouterStatus::HandlerFailed
    );

    EXPECT_EQ(
        result.response().status(),
        dispatcher::api::TransportStatus::InternalError
    );

    ASSERT_TRUE(result.response().error().has_value());
    EXPECT_EQ(result.response().error()->code(), "internal_error");
    EXPECT_EQ(result.response().error()->detail(), "handler exploded");
}

TEST(TransportRouterTests, ClearRemovesEndpointsAndHandlers)
{
    dispatcher::api::TransportRouter router;

    ASSERT_TRUE(
        router.add_handler(
            make_router_handler()
        ).ok()
    );

    ASSERT_FALSE(router.empty());

    router.clear();

    EXPECT_TRUE(router.empty());
    EXPECT_EQ(router.handler_count(), 0);
    EXPECT_TRUE(router.endpoints().empty());
    EXPECT_TRUE(router.endpoint_registry().empty());
}