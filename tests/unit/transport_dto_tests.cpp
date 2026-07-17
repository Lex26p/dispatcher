#include <dispatcher/api/transport_error.hpp>
#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_request_context.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_status.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <unordered_map>

TEST(TransportProtocolTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportProtocol::Unknown
        ),
        "unknown"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportProtocol::Http
        ),
        "http"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportProtocol::Grpc
        ),
        "grpc"
    );
}

TEST(TransportProtocolTests, PredicatesWork)
{
    EXPECT_FALSE(
        dispatcher::api::is_known_protocol(
            dispatcher::api::TransportProtocol::Unknown
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_known_protocol(
            dispatcher::api::TransportProtocol::Http
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_http_protocol(
            dispatcher::api::TransportProtocol::Http
        )
    );

    EXPECT_FALSE(
        dispatcher::api::is_http_protocol(
            dispatcher::api::TransportProtocol::Grpc
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_grpc_protocol(
            dispatcher::api::TransportProtocol::Grpc
        )
    );

    EXPECT_TRUE(
        dispatcher::api::supports_streaming(
            dispatcher::api::TransportProtocol::Grpc
        )
    );

    EXPECT_FALSE(
        dispatcher::api::supports_streaming(
            dispatcher::api::TransportProtocol::Http
        )
    );
}

TEST(TransportStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Ok
        ),
        "ok"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Created
        ),
        "created"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Accepted
        ),
        "accepted"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::BadRequest
        ),
        "bad_request"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Unauthorized
        ),
        "unauthorized"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Forbidden
        ),
        "forbidden"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Conflict
        ),
        "conflict"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::UnprocessableEntity
        ),
        "unprocessable_entity"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::InternalError
        ),
        "internal_error"
    );

    EXPECT_STREQ(
        dispatcher::api::to_string(
            dispatcher::api::TransportStatus::Unavailable
        ),
        "unavailable"
    );
}

TEST(TransportStatusTests, HttpStatusCodesWork)
{
    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Ok
        ),
        200
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Created
        ),
        201
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Accepted
        ),
        202
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::BadRequest
        ),
        400
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Unauthorized
        ),
        401
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Forbidden
        ),
        403
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::NotFound
        ),
        404
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Conflict
        ),
        409
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::UnprocessableEntity
        ),
        422
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::InternalError
        ),
        500
    );

    EXPECT_EQ(
        dispatcher::api::http_status_code(
            dispatcher::api::TransportStatus::Unavailable
        ),
        503
    );
}

TEST(TransportStatusTests, GrpcStatusCodesWork)
{
    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::Ok
        ),
        0
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::BadRequest
        ),
        3
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::NotFound
        ),
        5
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::Conflict
        ),
        6
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::Forbidden
        ),
        7
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::InternalError
        ),
        13
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::Unavailable
        ),
        14
    );

    EXPECT_EQ(
        dispatcher::api::grpc_status_code(
            dispatcher::api::TransportStatus::Unauthorized
        ),
        16
    );
}

TEST(TransportStatusTests, PredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportStatus::Ok
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportStatus::Created
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_success(
            dispatcher::api::TransportStatus::Accepted
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_client_error(
            dispatcher::api::TransportStatus::BadRequest
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_client_error(
            dispatcher::api::TransportStatus::Forbidden
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_server_error(
            dispatcher::api::TransportStatus::InternalError
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_server_error(
            dispatcher::api::TransportStatus::Unavailable
        )
    );

    EXPECT_TRUE(
        dispatcher::api::is_failure(
            dispatcher::api::TransportStatus::NotFound
        )
    );
}

TEST(TransportErrorTests, EmptyErrorWorks)
{
    const auto error = dispatcher::api::TransportError::none();

    EXPECT_TRUE(error.empty());
    EXPECT_FALSE(error.has_code());
    EXPECT_FALSE(error.has_message());
    EXPECT_FALSE(error.has_field());
    EXPECT_FALSE(error.has_detail());
}

TEST(TransportErrorTests, InvalidRequestErrorCapturesField)
{
    const auto error =
        dispatcher::api::TransportError::invalid_request(
            "path is invalid",
            "path",
            "/bad/path"
        );

    EXPECT_FALSE(error.empty());

    EXPECT_EQ(error.code(), "invalid_request");
    EXPECT_EQ(error.message(), "path is invalid");
    EXPECT_EQ(error.field(), "path");
    EXPECT_EQ(error.detail(), "/bad/path");

    EXPECT_TRUE(error.has_code());
    EXPECT_TRUE(error.has_message());
    EXPECT_TRUE(error.has_field());
    EXPECT_TRUE(error.has_detail());
}

TEST(TransportErrorTests, FactoryErrorsUseStableCodes)
{
    EXPECT_EQ(
        dispatcher::api::TransportError::unauthorized(
            "missing token"
        ).code(),
        "unauthorized"
    );

    EXPECT_EQ(
        dispatcher::api::TransportError::forbidden(
            "access denied"
        ).code(),
        "forbidden"
    );

    EXPECT_EQ(
        dispatcher::api::TransportError::not_found(
            "resource missing"
        ).code(),
        "not_found"
    );

    EXPECT_EQ(
        dispatcher::api::TransportError::conflict(
            "version conflict"
        ).code(),
        "conflict"
    );

    EXPECT_EQ(
        dispatcher::api::TransportError::internal_error(
            "unexpected error"
        ).code(),
        "internal_error"
    );
}

TEST(TransportRequestContextTests, CapturesFields)
{
    dispatcher::api::TransportRequestContext::Headers headers{
        {"content-type", "application/json"},
        {"x-request-id", "request-1"}
    };

    const dispatcher::api::TransportRequestContext context(
        dispatcher::api::TransportProtocol::Http,
        "GET",
        "/api/v1/runtime",
        "correlation-1",
        "operator-1",
        "127.0.0.1",
        headers
    );

    EXPECT_EQ(
        context.protocol(),
        dispatcher::api::TransportProtocol::Http
    );

    EXPECT_EQ(context.method(), "GET");
    EXPECT_EQ(context.path(), "/api/v1/runtime");
    EXPECT_EQ(context.correlation_id(), "correlation-1");
    EXPECT_EQ(context.operator_id(), "operator-1");
    EXPECT_EQ(context.remote_address(), "127.0.0.1");

    EXPECT_TRUE(context.has_correlation_id());
    EXPECT_TRUE(context.has_operator_id());
    EXPECT_TRUE(context.has_remote_address());
    EXPECT_TRUE(context.has_headers());

    EXPECT_EQ(context.header_count(), 2);

    EXPECT_TRUE(context.has_header("content-type"));

    const auto content_type = context.header("content-type");

    ASSERT_TRUE(content_type.has_value());
    EXPECT_EQ(content_type.value(), "application/json");

    EXPECT_FALSE(context.header("missing").has_value());

    EXPECT_TRUE(context.valid());
}

TEST(TransportRequestContextTests, InvalidWhenProtocolMethodOrPathMissing)
{
    const dispatcher::api::TransportRequestContext unknown_protocol(
        dispatcher::api::TransportProtocol::Unknown,
        "GET",
        "/api"
    );

    EXPECT_FALSE(unknown_protocol.valid());

    const dispatcher::api::TransportRequestContext missing_method(
        dispatcher::api::TransportProtocol::Http,
        "",
        "/api"
    );

    EXPECT_FALSE(missing_method.valid());

    const dispatcher::api::TransportRequestContext missing_path(
        dispatcher::api::TransportProtocol::Grpc,
        "Runtime/GetSnapshot",
        ""
    );

    EXPECT_FALSE(missing_path.valid());
}

TEST(TransportResponseTests, SuccessResponseWorks)
{
    const auto response =
        dispatcher::api::TransportResponse::success(
            "{\"ok\":true}"
        );

    EXPECT_TRUE(response.ok());
    EXPECT_FALSE(response.failed());

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::Ok
    );

    EXPECT_EQ(response.http_status(), 200);
    EXPECT_EQ(response.grpc_status(), 0);

    EXPECT_TRUE(response.has_body());
    EXPECT_EQ(response.body(), "{\"ok\":true}");

    EXPECT_TRUE(response.has_content_type());
    EXPECT_EQ(response.content_type(), "application/json");

    EXPECT_FALSE(response.has_error());
}

TEST(TransportResponseTests, CreatedAndAcceptedResponsesWork)
{
    const auto created =
        dispatcher::api::TransportResponse::created(
            "{\"created\":true}"
        );

    EXPECT_TRUE(created.ok());
    EXPECT_EQ(
        created.status(),
        dispatcher::api::TransportStatus::Created
    );

    EXPECT_EQ(created.http_status(), 201);

    const auto accepted =
        dispatcher::api::TransportResponse::accepted(
            "{\"accepted\":true}"
        );

    EXPECT_TRUE(accepted.ok());
    EXPECT_EQ(
        accepted.status(),
        dispatcher::api::TransportStatus::Accepted
    );

    EXPECT_EQ(accepted.http_status(), 202);
}

TEST(TransportResponseTests, FailureResponseWorks)
{
    const auto response =
        dispatcher::api::TransportResponse::failure(
            dispatcher::api::TransportStatus::BadRequest,
            dispatcher::api::TransportError::invalid_request(
                "request body is invalid",
                "body"
            )
        );

    EXPECT_FALSE(response.ok());
    EXPECT_TRUE(response.failed());

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::BadRequest
    );

    EXPECT_EQ(response.http_status(), 400);
    EXPECT_EQ(response.grpc_status(), 3);

    EXPECT_TRUE(response.has_error());

    ASSERT_TRUE(response.error().has_value());
    EXPECT_EQ(response.error()->code(), "invalid_request");
    EXPECT_EQ(response.error()->field(), "body");
}

TEST(TransportResponseTests, FailureConvertsSuccessStatusToInternalError)
{
    const auto response =
        dispatcher::api::TransportResponse::failure(
            dispatcher::api::TransportStatus::Ok,
            dispatcher::api::TransportError::internal_error(
                "bad caller status"
            )
        );

    EXPECT_TRUE(response.failed());

    EXPECT_EQ(
        response.status(),
        dispatcher::api::TransportStatus::InternalError
    );
}

TEST(TransportResponseTests, HeaderLookupWorks)
{
    dispatcher::api::TransportResponse::Headers headers{
        {"x-request-id", "request-1"},
        {"cache-control", "no-store"}
    };

    const dispatcher::api::TransportResponse response(
        dispatcher::api::TransportStatus::Ok,
        "{}",
        "application/json",
        headers
    );

    EXPECT_TRUE(response.has_headers());
    EXPECT_EQ(response.header_count(), 2);

    EXPECT_TRUE(response.has_header("x-request-id"));

    const auto request_id = response.header("x-request-id");

    ASSERT_TRUE(request_id.has_value());
    EXPECT_EQ(request_id.value(), "request-1");

    EXPECT_FALSE(response.header("missing").has_value());
}