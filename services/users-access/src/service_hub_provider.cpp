#include "dispatcher/users_access/service_hub_provider.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <chrono>
#include <cctype>
#include <initializer_list>
#include <memory>
#include <poll.h>
#include <string>
#include <thread>
#include <utility>

namespace dispatcher::users_access {
namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kEndpointPath = "/v1/ws";
constexpr int kReadPollMilliseconds = 100;
constexpr int kReconnectDelayMilliseconds = 200;

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

[[nodiscard]] bool object_has_only_fields(
    json_object* object,
    const std::initializer_list<std::string_view> allowed) {
    if (object == nullptr || !json_object_is_type(object, json_type_object)) {
        return false;
    }

    json_object_object_foreach(object, key, value) {
        (void)value;
        bool accepted = false;
        for (const auto candidate : allowed) {
            if (candidate == key) {
                accepted = true;
                break;
            }
        }
        if (!accepted) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string success_response(
    const std::string_view request_id,
    json_object* payload) {
    auto response = adopt_json(json_object_new_object());
    json_object_object_add(response.get(), "type", json_object_new_string("response"));
    json_object_object_add(
        response.get(),
        "id",
        json_object_new_string_len(request_id.data(), static_cast<int>(request_id.size())));
    json_object_object_add(response.get(), "ok", json_object_new_boolean(1));
    json_object_object_add(response.get(), "payload", payload);
    return serialize(response.get());
}

[[nodiscard]] std::string empty_success_response(
    const std::string_view request_id) {
    return success_response(request_id, json_object_new_object());
}

[[nodiscard]] std::string error_response(
    const std::string_view request_id,
    const std::string_view code,
    const std::string_view message) {
    auto error = adopt_json(json_object_new_object());
    json_object_object_add(
        error.get(),
        "code",
        json_object_new_string_len(code.data(), static_cast<int>(code.size())));
    json_object_object_add(
        error.get(),
        "message",
        json_object_new_string_len(message.data(), static_cast<int>(message.size())));

    auto response = adopt_json(json_object_new_object());
    json_object_object_add(response.get(), "type", json_object_new_string("response"));
    json_object_object_add(
        response.get(),
        "id",
        json_object_new_string_len(request_id.data(), static_cast<int>(request_id.size())));
    json_object_object_add(response.get(), "ok", json_object_new_boolean(0));
    json_object_object_add(response.get(), "error", error.release());
    return serialize(response.get());
}

[[nodiscard]] json_object* user_json(const User& user) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, "id", json_object_new_string(user.id.c_str()));
    json_object_object_add(object, "login", json_object_new_string(user.login.c_str()));
    json_object_object_add(
        object,
        "display_name",
        json_object_new_string(user.display_name.c_str()));
    json_object_object_add(object, "enabled", json_object_new_boolean(user.enabled));
    return object;
}

[[nodiscard]] json_object* session_json(const AuthenticatedSession& session) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, "user", user_json(session.user));
    json_object_object_add(
        object,
        "issued_at_unix_ms",
        json_object_new_int64(session.issued_at_unix_ms));
    json_object_object_add(
        object,
        "absolute_expires_at_unix_ms",
        json_object_new_int64(session.absolute_expires_at_unix_ms));
    json_object_object_add(
        object,
        "idle_timeout_ms",
        json_object_new_int64(session.idle_timeout_ms));
    return object;
}

[[nodiscard]] std::pair<std::string_view, std::string_view> session_error_info(
    const AuthenticationSessionError error) {
    switch (error) {
    case AuthenticationSessionError::invalid_credentials:
        return {"auth.invalid_credentials", "Authentication credentials were rejected"};
    case AuthenticationSessionError::invalid_session:
    case AuthenticationSessionError::user_disabled:
        return {"auth.invalid_session", "Authenticated session is invalid"};
    case AuthenticationSessionError::session_expired:
        return {"auth.session_expired", "Authenticated session has expired"};
    case AuthenticationSessionError::storage_error:
        return {"access.storage_error", "Users & Access storage operation failed"};
    case AuthenticationSessionError::crypto_error:
    case AuthenticationSessionError::session_generation_failed:
        return {"auth.crypto_error", "Authentication cryptographic operation failed"};
    case AuthenticationSessionError::none:
        break;
    }
    return {"access.internal_error", "Users & Access operation failed"};
}

[[nodiscard]] std::string session_error_response(
    const std::string_view request_id,
    const AuthenticationSessionError error) {
    const auto [code, message] = session_error_info(error);
    return error_response(request_id, code, message);
}

[[nodiscard]] bool session_auth_token(
    json_object* message,
    std::string& token) {
    json_object* auth = nullptr;
    if (!json_object_object_get_ex(message, "auth", &auth) ||
        !object_has_only_fields(auth, {"type", "token"})) {
        return false;
    }

    std::string type;
    return string_field(auth, "type", type) && type == "session" &&
           string_field(auth, "token", token);
}

[[nodiscard]] std::optional<Capability> parse_capability(
    const std::string_view name) {
    for (const auto capability : all_capabilities) {
        if (capability_name(capability) == name) {
            return capability;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<AccessScope> parse_scope(json_object* value) {
    if (!object_has_only_fields(value, {"kind", "project_id"})) {
        return std::nullopt;
    }

    std::string kind;
    if (!string_field(value, "kind", kind)) {
        return std::nullopt;
    }

    json_object* project_id_value = nullptr;
    const bool has_project_id =
        json_object_object_get_ex(value, "project_id", &project_id_value);

    if (kind == "global") {
        if (has_project_id) {
            return std::nullopt;
        }
        return AccessScope::global();
    }

    if (kind != "project" || !has_project_id ||
        !json_object_is_type(project_id_value, json_type_string)) {
        return std::nullopt;
    }

    const std::string project_id = json_object_get_string(project_id_value);
    if (project_id.empty()) {
        return std::nullopt;
    }
    return AccessScope::project(project_id);
}

[[nodiscard]] json_object* access_evaluation_json(
    const AccessEvaluation& evaluation) {
    json_object* object = json_object_new_object();
    json_object_object_add(
        object,
        "allowed",
        json_object_new_boolean(evaluation.allowed));

    json_object* capabilities = json_object_new_array();
    for (const auto capability : evaluation.effective_capabilities) {
        const auto name = capability_name(capability);
        json_object_array_add(
            capabilities,
            json_object_new_string_len(name.data(), static_cast<int>(name.size())));
    }
    json_object_object_add(object, "effective_capabilities", capabilities);
    return object;
}

[[nodiscard]] bool is_staged_administration_operation(
    const std::string_view operation) {
    return operation == contract::list_users ||
           operation == contract::create_user ||
           operation == contract::set_user_enabled ||
           operation == contract::set_user_password ||
           operation == contract::list_permission_sets ||
           operation == contract::create_permission_set ||
           operation == contract::list_access_assignments ||
           operation == contract::assign_access ||
           operation == contract::remove_access_assignment;
}

[[nodiscard]] std::string handle_request(
    AuthenticationSessionService& authentication,
    const std::string_view request_id,
    const std::string_view operation,
    json_object* message,
    json_object* payload) {
    if (operation == contract::login) {
        if (!object_has_only_fields(payload, {"login", "password"})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "login payload must contain only login and password");
        }

        std::string login;
        std::string password;
        if (!string_field(payload, "login", login) ||
            !string_field(payload, "password", password)) {
            return error_response(
                request_id,
                "access.invalid_request",
                "login requires string login and password");
        }

        auto result = authentication.login(login, password);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }

        auto response = adopt_json(json_object_new_object());
        json_object_object_add(
            response.get(),
            "session_token",
            json_object_new_string(result.value->token.c_str()));
        json_object_object_add(
            response.get(),
            "session",
            session_json(result.value->session));
        return success_response(request_id, response.release());
    }

    std::string token;
    if (!session_auth_token(message, token)) {
        return error_response(
            request_id,
            "auth.invalid_session",
            "Protected Users & Access operation requires session authentication");
    }

    if (operation == contract::logout) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "logout payload must be an empty object");
        }
        const auto error = authentication.logout(token);
        return error == AuthenticationSessionError::none
            ? empty_success_response(request_id)
            : session_error_response(request_id, error);
    }

    if (operation == contract::current_session) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "current-session payload must be an empty object");
        }
        auto result = authentication.validate(token);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object_object_add(
            response.get(),
            "session",
            session_json(*result.value));
        return success_response(request_id, response.release());
    }

    if (operation == contract::evaluate_access) {
        if (!object_has_only_fields(payload, {"scope", "capability"})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access requires scope and capability");
        }

        json_object* scope_value = nullptr;
        std::string capability_name_value;
        if (!json_object_object_get_ex(payload, "scope", &scope_value) ||
            !string_field(payload, "capability", capability_name_value)) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access requires scope and capability");
        }

        const auto scope = parse_scope(scope_value);
        const auto capability = parse_capability(capability_name_value);
        if (!scope.has_value() || !capability.has_value()) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access scope or capability is invalid");
        }

        auto result = authentication.evaluate_access(token, *scope, *capability);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }
        return success_response(
            request_id,
            access_evaluation_json(*result.value));
    }

    if (is_staged_administration_operation(operation)) {
        auto admin = authentication.evaluate_access(
            token,
            AccessScope::global(),
            Capability::admin);
        if (!admin.ok()) {
            return session_error_response(request_id, admin.error);
        }
        if (!admin.value->allowed) {
            return error_response(
                request_id,
                "access.forbidden",
                "Global admin capability is required");
        }
        return error_response(
            request_id,
            "access.internal_error",
            "Administration operation implementation is staged for CORE-005 Step 6");
    }

    return error_response(
        request_id,
        "access.unknown_operation",
        "Users & Access does not support the requested operation");
}

class HubConnection final {
public:
    HubConnection()
        : resolver_(io_context_), websocket_(io_context_) {}

    [[nodiscard]] bool connect(const ServiceHubEndpoint& endpoint) {
        beast::error_code error;
        const auto endpoints = resolver_.resolve(endpoint.host, endpoint.port, error);
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
                    request.set(http::field::sec_websocket_protocol, kSubprotocol);
                }));

        websocket::response_type response;
        websocket_.handshake(
            response,
            endpoint.host + ":" + endpoint.port,
            std::string(kEndpointPath),
            error);
        if (error) {
            return false;
        }

        const auto negotiated = response[http::field::sec_websocket_protocol];
        return std::string_view(negotiated.data(), negotiated.size()) == kSubprotocol;
    }

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    enum class ReadResult {
        timeout,
        message,
        closed,
    };

    [[nodiscard]] ReadResult read(std::string& message, const int timeout_ms) {
        pollfd descriptor{
            beast::get_lowest_layer(websocket_).native_handle(),
            POLLIN,
            0};
        const int poll_result = ::poll(&descriptor, 1, timeout_ms);
        if (poll_result == 0) {
            return ReadResult::timeout;
        }
        if (poll_result < 0 ||
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
            (descriptor.revents & POLLIN) == 0) {
            return ReadResult::closed;
        }

        beast::flat_buffer buffer;
        beast::error_code error;
        websocket_.read(buffer, error);
        if (error || !websocket_.got_text()) {
            return ReadResult::closed;
        }
        message = beast::buffers_to_string(buffer.data());
        return ReadResult::message;
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

[[nodiscard]] bool register_provider(HubConnection& connection) {
    auto registration = adopt_json(json_object_new_object());
    json_object_object_add(registration.get(), "type", json_object_new_string("register"));
    json_object_object_add(
        registration.get(),
        "service",
        json_object_new_string(std::string(ServiceHubProvider::service_address).c_str()));
    if (!connection.write(serialize(registration.get()))) {
        return false;
    }

    std::string response_text;
    if (connection.read(response_text, 3000) != HubConnection::ReadResult::message) {
        return false;
    }

    auto response = adopt_json(json_tokener_parse(response_text.c_str()));
    if (!response || !json_object_is_type(response.get(), json_type_object)) {
        return false;
    }

    std::string type;
    std::string service;
    return string_field(response.get(), "type", type) && type == "registered" &&
           string_field(response.get(), "service", service) &&
           service == ServiceHubProvider::service_address;
}

[[nodiscard]] bool handle_hub_message(
    HubConnection& connection,
    AuthenticationSessionService& authentication,
    const std::string& message_text) {
    auto message = adopt_json(json_tokener_parse(message_text.c_str()));
    if (!message || !json_object_is_type(message.get(), json_type_object)) {
        return false;
    }

    std::string type;
    if (!string_field(message.get(), "type", type)) {
        return false;
    }
    if (type == "cancel") {
        return true;
    }
    if (type != "request") {
        return false;
    }

    std::string request_id;
    std::string service;
    std::string operation;
    json_object* payload = nullptr;
    if (!string_field(message.get(), "id", request_id) ||
        !string_field(message.get(), "service", service) ||
        service != ServiceHubProvider::service_address ||
        !string_field(message.get(), "operation", operation) ||
        !json_object_object_get_ex(message.get(), "payload", &payload)) {
        return false;
    }

    return connection.write(handle_request(
        authentication,
        request_id,
        operation,
        message.get(),
        payload));
}

void interruptible_sleep(const std::atomic<bool>& stop_requested) {
    constexpr int slices = kReconnectDelayMilliseconds / 20;
    for (int slice = 0; slice < slices && !stop_requested.load(); ++slice) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

}  // namespace

std::optional<ServiceHubEndpoint> parse_service_hub_address(
    const std::string_view address) {
    const auto separator = address.rfind(':');
    if (separator == std::string_view::npos || separator == 0 ||
        separator + 1 >= address.size()) {
        return std::nullopt;
    }

    const std::string_view host = address.substr(0, separator);
    const std::string_view port = address.substr(separator + 1);
    unsigned int port_value = 0;
    for (const unsigned char character : port) {
        if (!std::isdigit(character)) {
            return std::nullopt;
        }
        port_value = port_value * 10U + static_cast<unsigned int>(character - '0');
        if (port_value > 65535U) {
            return std::nullopt;
        }
    }
    if (port_value == 0) {
        return std::nullopt;
    }

    return ServiceHubEndpoint{
        .host = std::string(host),
        .port = std::string(port),
    };
}

ServiceHubProvider::ServiceHubProvider(
    AuthenticationSessionService& authentication,
    ServiceHubEndpoint endpoint)
    : authentication_(authentication),
      endpoint_(std::move(endpoint)) {}

ServiceHubProvider::~ServiceHubProvider() {
    stop();
}

bool ServiceHubProvider::start() {
    if (worker_.joinable()) {
        return false;
    }
    stop_requested_.store(false);
    try {
        worker_ = std::thread([this] { run(); });
    } catch (...) {
        return false;
    }
    return true;
}

void ServiceHubProvider::stop() {
    stop_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
}

void ServiceHubProvider::run() {
    while (!stop_requested_.load()) {
        HubConnection connection;
        if (!connection.connect(endpoint_) || !register_provider(connection)) {
            connection.close();
            interruptible_sleep(stop_requested_);
            continue;
        }

        bool healthy = true;
        while (healthy && !stop_requested_.load()) {
            std::string message;
            switch (connection.read(message, kReadPollMilliseconds)) {
            case HubConnection::ReadResult::timeout:
                break;
            case HubConnection::ReadResult::closed:
                healthy = false;
                break;
            case HubConnection::ReadResult::message:
                healthy = handle_hub_message(connection, authentication_, message);
                break;
            }
        }

        connection.close();
        if (!stop_requested_.load()) {
            interruptible_sleep(stop_requested_);
        }
    }
}

}  // namespace dispatcher::users_access
