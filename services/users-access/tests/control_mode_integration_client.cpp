#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <poll.h>
#include <string>
#include <string_view>
#include <thread>

namespace {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kService = "users-access.v1";
constexpr std::string_view kLogin = "integration-admin";
constexpr std::string_view kPassword = "integration password 123";

int fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

[[nodiscard]] JsonPtr parse_json(const std::string& text) {
    return JsonPtr(json_tokener_parse(text.c_str()), &json_object_put);
}

[[nodiscard]] bool string_field(
    json_object* object,
    const char* name,
    std::string& value) {
    json_object* field = nullptr;
    if (!json_object_object_get_ex(object, name, &field) || field == nullptr ||
        !json_object_is_type(field, json_type_string)) {
        return false;
    }
    value = json_object_get_string(field);
    return true;
}

class Client final {
public:
    Client() : resolver_(io_context_), websocket_(io_context_) {}

    [[nodiscard]] bool connect(const std::string& host, const std::string& port) {
        beast::error_code error;
        const auto endpoints = resolver_.resolve(host, port, error);
        if (error) return false;
        asio::connect(beast::get_lowest_layer(websocket_), endpoints, error);
        if (error) return false;
        websocket_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& request) {
                request.set(http::field::sec_websocket_protocol, kSubprotocol);
            }));
        websocket::response_type response;
        websocket_.handshake(response, host + ":" + port, "/v1/ws", error);
        if (error) return false;
        const auto protocol = response[http::field::sec_websocket_protocol];
        return std::string_view(protocol.data(), protocol.size()) == kSubprotocol;
    }

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(std::string& message, const int timeout_ms = 3000) {
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
        if (error || !websocket_.got_text()) return false;
        message = beast::buffers_to_string(buffer.data());
        return true;
    }

private:
    asio::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> websocket_;
};

[[nodiscard]] std::string request(
    const std::string_view id,
    const std::string_view operation,
    const std::string_view payload_json,
    const std::string_view session_token = {}) {
    std::string result =
        std::string(R"({"type":"request","id":")") + std::string(id) +
        R"(","service":")" + std::string(kService) +
        R"(","operation":")" + std::string(operation) +
        R"(","payload":)" + std::string(payload_json);
    if (!session_token.empty()) {
        result += R"(,"auth":{"type":"session","token":")" +
                  std::string(session_token) + R"("})";
    }
    result += R"(,"timeout_ms":5000})";
    return result;
}

[[nodiscard]] bool is_error(
    const std::string& text,
    const std::string_view id,
    const std::string_view code) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string response_id;
    json_object* ok = nullptr;
    json_object* error = nullptr;
    std::string actual_code;
    return string_field(object.get(), "type", type) && type == "response" &&
           string_field(object.get(), "id", response_id) && response_id == id &&
           json_object_object_get_ex(object.get(), "ok", &ok) && ok != nullptr &&
           json_object_is_type(ok, json_type_boolean) && json_object_get_boolean(ok) == 0 &&
           json_object_object_get_ex(object.get(), "error", &error) && error != nullptr &&
           string_field(error, "code", actual_code) && actual_code == code;
}

[[nodiscard]] bool login_response(
    const std::string& text,
    const std::string_view expected_id,
    std::string& token) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string response_id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    return string_field(object.get(), "type", type) && type == "response" &&
           string_field(object.get(), "id", response_id) && response_id == expected_id &&
           json_object_object_get_ex(object.get(), "ok", &ok) && ok != nullptr &&
           json_object_get_boolean(ok) != 0 &&
           json_object_object_get_ex(object.get(), "payload", &payload) && payload != nullptr &&
           string_field(payload, "session_token", token) && token.size() == 64;
}

[[nodiscard]] bool empty_success_response(
    const std::string& text,
    const std::string_view expected_id) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string response_id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    return string_field(object.get(), "type", type) && type == "response" &&
           string_field(object.get(), "id", response_id) && response_id == expected_id &&
           json_object_object_get_ex(object.get(), "ok", &ok) && ok != nullptr &&
           json_object_get_boolean(ok) != 0 &&
           json_object_object_get_ex(object.get(), "payload", &payload) && payload != nullptr &&
           json_object_is_type(payload, json_type_object) &&
           json_object_object_length(payload) == 0;
}

struct ControlModeView final {
    bool enabled{false};
    std::string reason;
    std::string project_id;
    std::int64_t expires_at_unix_ms{0};
};

[[nodiscard]] bool control_mode_response(
    const std::string& text,
    const std::string_view expected_id,
    ControlModeView& view) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string response_id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* mode = nullptr;
    json_object* enabled = nullptr;
    if (!string_field(object.get(), "type", type) || type != "response" ||
        !string_field(object.get(), "id", response_id) || response_id != expected_id ||
        !json_object_object_get_ex(object.get(), "ok", &ok) || ok == nullptr ||
        json_object_get_boolean(ok) == 0 ||
        !json_object_object_get_ex(object.get(), "payload", &payload) || payload == nullptr ||
        !json_object_object_get_ex(payload, "control_mode", &mode) || mode == nullptr ||
        !json_object_object_get_ex(mode, "enabled", &enabled) || enabled == nullptr ||
        !json_object_is_type(enabled, json_type_boolean) ||
        !string_field(mode, "reason", view.reason)) {
        return false;
    }

    view.enabled = json_object_get_boolean(enabled) != 0;
    view.project_id.clear();
    view.expires_at_unix_ms = 0;
    if (!view.enabled) {
        return true;
    }

    json_object* expires = nullptr;
    if (!string_field(mode, "project_id", view.project_id) || view.project_id.empty() ||
        !json_object_object_get_ex(mode, "expires_at_unix_ms", &expires) || expires == nullptr ||
        !json_object_is_type(expires, json_type_int)) {
        return false;
    }
    view.expires_at_unix_ms = json_object_get_int64(expires);
    return view.expires_at_unix_ms > 0;
}

[[nodiscard]] bool wait_for_provider(Client& client) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        const std::string id = "probe-" + std::to_string(attempt);
        if (!client.write(request(id, "current-control-mode", "{}"))) return false;
        std::string response;
        if (!client.read(response)) return false;
        if (is_error(response, id, "auth.invalid_session")) return true;
        if (!is_error(response, id, "hub.unknown_service")) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: users-access-control-mode-integration-client <host> <port>\n";
        return 2;
    }

    Client client;
    if (!client.connect(argv[1], argv[2])) return fail("connect to Service Hub");
    if (!wait_for_provider(client)) return fail("wait for Users & Access provider registration");

    std::string response;
    if (!client.write(request("unauth", "current-control-mode", "{}")) ||
        !client.read(response) ||
        !is_error(response, "unauth", "auth.invalid_session")) {
        return fail("control-mode status should require authentication");
    }

    const std::string login_payload =
        std::string(R"({"login":")") + std::string(kLogin) +
        R"(","password":")" + std::string(kPassword) + R"("})";
    if (!client.write(request("login", "login", login_payload)) ||
        !client.read(response)) {
        return fail("login request");
    }

    std::string token;
    if (!login_response(response, "login", token)) {
        return fail("login response");
    }

    ControlModeView view;
    if (!client.write(request("initial", "current-control-mode", "{}", token)) ||
        !client.read(response) ||
        !control_mode_response(response, "initial", view) ||
        view.enabled || view.reason != "inactive") {
        return fail("new authenticated session should start inactive");
    }

    if (!client.write(request(
            "enable",
            "enable-control-mode",
            R"({"project_id":"integration-project"})",
            token)) ||
        !client.read(response) ||
        !control_mode_response(response, "enable", view) ||
        !view.enabled || view.reason != "enabled" ||
        view.project_id != "integration-project") {
        return fail("global control-capable admin should enable project control mode");
    }

    const std::int64_t expiry = view.expires_at_unix_ms;
    if (!client.write(request("current", "current-control-mode", "{}", token)) ||
        !client.read(response) ||
        !control_mode_response(response, "current", view) ||
        !view.enabled || view.expires_at_unix_ms != expiry) {
        return fail("control-mode status should preserve the absolute expiry");
    }

    if (!client.write(request("disable", "disable-control-mode", "{}", token)) ||
        !client.read(response) ||
        !control_mode_response(response, "disable", view) ||
        view.enabled || view.reason != "inactive") {
        return fail("explicit disable should clear control mode");
    }

    if (!client.write(request(
            "enable-again",
            "enable-control-mode",
            R"({"project_id":"integration-project"})",
            token)) ||
        !client.read(response) ||
        !control_mode_response(response, "enable-again", view) ||
        !view.enabled) {
        return fail("control mode should enable before logout reset check");
    }

    if (!client.write(request("logout", "logout", "{}", token)) ||
        !client.read(response) ||
        !empty_success_response(response, "logout")) {
        return fail("logout");
    }

    if (!client.write(request("after-logout", "current-control-mode", "{}", token)) ||
        !client.read(response) ||
        !is_error(response, "after-logout", "auth.invalid_session")) {
        return fail("logout must invalidate control mode with the session");
    }

    std::cout << "Users & Access control mode Service Hub integration passed\n";
    return 0;
}
