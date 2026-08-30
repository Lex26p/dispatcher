#include "dispatcher/project_manager/authorization_client.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <poll.h>
#include <string>
#include <utility>

namespace dispatcher::project_manager {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kEndpointPath = "/v1/ws";
constexpr std::string_view kUsersAccessService = "users-access.v1";
constexpr std::string_view kEvaluateAccessOperation = "evaluate-access";
constexpr int kRequestTimeoutMs = 3000;
constexpr int kReadTimeoutMs = 3500;

using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

[[nodiscard]] JsonPtr adopt_json(json_object* value) {
    return JsonPtr(value, &json_object_put);
}

[[nodiscard]] std::string serialize(json_object* value) {
    return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
}

[[nodiscard]] bool string_field(
    json_object* object,
    const char* name,
    std::string& value) {
    json_object* field = nullptr;
    if (!json_object_object_get_ex(object, name, &field) ||
        field == nullptr ||
        !json_object_is_type(field, json_type_string)) {
        return false;
    }

    value = json_object_get_string(field);
    return true;
}

class ClientConnection final {
public:
    ClientConnection()
        : resolver_(io_context_),
          websocket_(io_context_) {}

    [[nodiscard]] bool connect(
        const std::string& host,
        const std::string& port) {
        beast::error_code error;
        const auto endpoints = resolver_.resolve(host, port, error);
        if (error) {
            return false;
        }

        asio::connect(beast::get_lowest_layer(websocket_), endpoints, error);
        if (error) {
            return false;
        }

        websocket_.set_option(
            websocket::stream_base::decorator(
                [](websocket::request_type& request) {
                    request.set(
                        http::field::sec_websocket_protocol,
                        kSubprotocol);
                }));

        websocket::response_type response;
        websocket_.handshake(
            response,
            host + ":" + port,
            std::string(kEndpointPath),
            error);
        if (error) {
            return false;
        }

        const auto negotiated =
            response[http::field::sec_websocket_protocol];
        return std::string_view(
                   negotiated.data(),
                   negotiated.size()) == kSubprotocol;
    }

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(
        std::string& message,
        const int timeout_ms) {
        pollfd descriptor{
            beast::get_lowest_layer(websocket_).native_handle(),
            POLLIN,
            0};

        const int result = ::poll(&descriptor, 1, timeout_ms);
        if (result <= 0 ||
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
            (descriptor.revents & POLLIN) == 0) {
            return false;
        }

        beast::flat_buffer buffer;
        beast::error_code error;
        websocket_.read(buffer, error);
        if (error || !websocket_.got_text()) {
            return false;
        }

        message = beast::buffers_to_string(buffer.data());
        return true;
    }

    void close() {
        beast::error_code error;
        websocket_.close(websocket::close_code::normal, error);
        if (error) {
            auto& socket = beast::get_lowest_layer(websocket_);
            error = {};
            socket.close(error);
        }
    }

private:
    asio::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> websocket_;
};

[[nodiscard]] json_object* scope_json(
    const AuthorizationScope& scope) {
    json_object* value = json_object_new_object();

    if (scope.kind == AuthorizationScopeKind::global) {
        json_object_object_add(
            value,
            "kind",
            json_object_new_string("global"));
        return value;
    }

    json_object_object_add(
        value,
        "kind",
        json_object_new_string("project"));
    json_object_object_add(
        value,
        "project_id",
        json_object_new_string(scope.project_id.c_str()));
    return value;
}

}  // namespace

class UsersAccessAuthorizationClient::Impl final {
public:
    Impl(std::string host, std::string port)
        : host_(std::move(host)),
          port_(std::move(port)) {}

    [[nodiscard]] AuthorizationResult evaluate(
        const std::string_view session_token,
        const AuthorizationScope& scope,
        const std::string_view capability) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            if (!ensure_connected()) {
                reset();
                continue;
            }

            const auto result =
                evaluate_connected(session_token, scope, capability);
            if (result.has_value()) {
                return *result;
            }

            reset();
        }

        return AuthorizationResult::unavailable;
    }

private:
    [[nodiscard]] bool ensure_connected() {
        if (connection_) {
            return true;
        }

        auto connection = std::make_unique<ClientConnection>();
        if (!connection->connect(host_, port_)) {
            return false;
        }

        connection_ = std::move(connection);
        return true;
    }

    void reset() {
        if (connection_) {
            connection_->close();
            connection_.reset();
        }
    }

    [[nodiscard]] std::optional<AuthorizationResult> evaluate_connected(
        const std::string_view session_token,
        const AuthorizationScope& scope,
        const std::string_view capability) {
        const std::string request_id =
            "project-auth-" + std::to_string(next_request_id_++);

        auto request = adopt_json(json_object_new_object());
        json_object_object_add(
            request.get(),
            "type",
            json_object_new_string("request"));
        json_object_object_add(
            request.get(),
            "id",
            json_object_new_string(request_id.c_str()));
        json_object_object_add(
            request.get(),
            "service",
            json_object_new_string(
                std::string(kUsersAccessService).c_str()));
        json_object_object_add(
            request.get(),
            "operation",
            json_object_new_string(
                std::string(kEvaluateAccessOperation).c_str()));

        auto payload = adopt_json(json_object_new_object());
        json_object_object_add(
            payload.get(),
            "scope",
            scope_json(scope));
        json_object_object_add(
            payload.get(),
            "capability",
            json_object_new_string_len(
                capability.data(),
                static_cast<int>(capability.size())));
        json_object_object_add(
            request.get(),
            "payload",
            payload.release());

        auto auth = adopt_json(json_object_new_object());
        json_object_object_add(
            auth.get(),
            "type",
            json_object_new_string("session"));
        json_object_object_add(
            auth.get(),
            "token",
            json_object_new_string_len(
                session_token.data(),
                static_cast<int>(session_token.size())));
        json_object_object_add(
            request.get(),
            "auth",
            auth.release());
        json_object_object_add(
            request.get(),
            "timeout_ms",
            json_object_new_int(kRequestTimeoutMs));

        if (!connection_->write(serialize(request.get()))) {
            return std::nullopt;
        }

        std::string response_text;
        if (!connection_->read(response_text, kReadTimeoutMs)) {
            return std::nullopt;
        }

        auto response = adopt_json(
            json_tokener_parse(response_text.c_str()));
        if (!response ||
            !json_object_is_type(
                response.get(),
                json_type_object)) {
            return std::nullopt;
        }

        std::string type;
        std::string response_id;
        json_object* ok = nullptr;
        if (!string_field(response.get(), "type", type) ||
            type != "response" ||
            !string_field(response.get(), "id", response_id) ||
            response_id != request_id ||
            !json_object_object_get_ex(
                response.get(),
                "ok",
                &ok) ||
            ok == nullptr ||
            !json_object_is_type(ok, json_type_boolean)) {
            return std::nullopt;
        }

        if (json_object_get_boolean(ok) != 0) {
            json_object* response_payload = nullptr;
            json_object* allowed = nullptr;
            if (!json_object_object_get_ex(
                    response.get(),
                    "payload",
                    &response_payload) ||
                response_payload == nullptr ||
                !json_object_is_type(
                    response_payload,
                    json_type_object) ||
                !json_object_object_get_ex(
                    response_payload,
                    "allowed",
                    &allowed) ||
                allowed == nullptr ||
                !json_object_is_type(
                    allowed,
                    json_type_boolean)) {
                return std::nullopt;
            }

            return json_object_get_boolean(allowed) != 0
                ? AuthorizationResult::allowed
                : AuthorizationResult::denied;
        }

        json_object* error = nullptr;
        std::string code;
        if (!json_object_object_get_ex(
                response.get(),
                "error",
                &error) ||
            error == nullptr ||
            !json_object_is_type(error, json_type_object) ||
            !string_field(error, "code", code)) {
            return std::nullopt;
        }

        if (code == "auth.invalid_session") {
            return AuthorizationResult::invalid_session;
        }
        if (code == "auth.session_expired") {
            return AuthorizationResult::session_expired;
        }

        return AuthorizationResult::unavailable;
    }

    std::string host_;
    std::string port_;
    std::unique_ptr<ClientConnection> connection_;
    std::uint64_t next_request_id_{1};
};

UsersAccessAuthorizationClient::UsersAccessAuthorizationClient(
    std::string host,
    std::string port)
    : impl_(std::make_unique<Impl>(
          std::move(host),
          std::move(port))) {}

UsersAccessAuthorizationClient::~UsersAccessAuthorizationClient() = default;

AuthorizationResult UsersAccessAuthorizationClient::evaluate(
    const std::string_view session_token,
    const AuthorizationScope& scope,
    const std::string_view capability) {
    return impl_->evaluate(session_token, scope, capability);
}

}  // namespace dispatcher::project_manager
