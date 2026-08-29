#include "dispatcher/service_hub/server.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <chrono>
#include <iostream>
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

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

class TestWebSocket final {
public:
    TestWebSocket()
        : resolver_(io_context_),
          websocket_(io_context_) {}

    [[nodiscard]] bool connect(
        const std::string& host,
        const int port) {
        beast::error_code error;

        const auto endpoints = resolver_.resolve(
            host,
            std::to_string(port),
            error);

        if (error) {
            return false;
        }

        asio::connect(
            beast::get_lowest_layer(websocket_),
            endpoints,
            error);

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

        websocket_.handshake(
            host + ":" + std::to_string(port),
            "/v1/ws",
            error);

        return !error;
    }

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(
        std::string& message,
        const int timeout_ms = 3000) {
        pollfd descriptor{
            beast::get_lowest_layer(websocket_).native_handle(),
            POLLIN,
            0};

        const int poll_result = ::poll(
            &descriptor,
            1,
            timeout_ms);

        if (poll_result <= 0 ||
            (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
            (descriptor.revents & POLLIN) == 0) {
            return false;
        }

        beast::flat_buffer buffer;
        beast::error_code error;
        websocket_.read(buffer, error);

        if (error) {
            return false;
        }

        message = beast::buffers_to_string(buffer.data());
        return websocket_.got_text();
    }

    void abort() {
        beast::error_code error;
        auto& socket = beast::get_lowest_layer(websocket_);
        socket.cancel(error);
        error = {};
        socket.close(error);
    }

private:
    asio::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> websocket_;
};

[[nodiscard]] bool json_string_field_equals(
    json_object* object,
    const char* field,
    const std::string_view expected) {
    json_object* value = nullptr;

    if (!json_object_object_get_ex(object, field, &value) ||
        !json_object_is_type(value, json_type_string)) {
        return false;
    }

    return std::string_view(json_object_get_string(value)) == expected;
}

[[nodiscard]] bool response_has_error(
    const std::string& message,
    const std::string_view expected_id,
    const std::string_view expected_code) {
    json_object* response = json_tokener_parse(message.c_str());

    if (response == nullptr) {
        return false;
    }

    json_object* ok = nullptr;
    json_object* error = nullptr;
    json_object* code = nullptr;

    const bool valid =
        json_string_field_equals(response, "type", "response") &&
        json_string_field_equals(response, "id", expected_id) &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) == 0 &&
        json_object_object_get_ex(response, "error", &error) &&
        json_object_is_type(error, json_type_object) &&
        json_object_object_get_ex(error, "code", &code) &&
        json_object_is_type(code, json_type_string) &&
        std::string_view(json_object_get_string(code)) == expected_code;

    json_object_put(response);
    return valid;
}

[[nodiscard]] bool response_is_success(
    const std::string& message,
    const std::string_view expected_id,
    const std::string_view expected_text) {
    json_object* response = json_tokener_parse(message.c_str());

    if (response == nullptr) {
        return false;
    }

    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* text = nullptr;

    const bool valid =
        json_string_field_equals(response, "type", "response") &&
        json_string_field_equals(response, "id", expected_id) &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) != 0 &&
        json_object_object_get_ex(response, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "text", &text) &&
        json_object_is_type(text, json_type_string) &&
        std::string_view(json_object_get_string(text)) == expected_text;

    json_object_put(response);
    return valid;
}

[[nodiscard]] bool register_provider(
    TestWebSocket& provider,
    const int port,
    const std::string_view service) {
    if (!provider.connect("127.0.0.1", port)) {
        return false;
    }

    const std::string registration =
        R"({"type":"register","service":")" +
        std::string(service) +
        R"("})";

    if (!provider.write(registration)) {
        return false;
    }

    std::string response;

    if (!provider.read(response)) {
        return false;
    }

    json_object* registered = json_tokener_parse(response.c_str());

    if (registered == nullptr) {
        return false;
    }

    const bool valid =
        json_string_field_equals(registered, "type", "registered") &&
        json_string_field_equals(registered, "service", service);

    json_object_put(registered);
    return valid;
}

[[nodiscard]] bool extract_request_id(
    const std::string& message,
    std::string& request_id) {
    json_object* request = json_tokener_parse(message.c_str());

    if (request == nullptr) {
        return false;
    }

    json_object* id = nullptr;

    const bool valid =
        json_string_field_equals(request, "type", "request") &&
        json_object_object_get_ex(request, "id", &id) &&
        json_object_is_type(id, json_type_string);

    if (valid) {
        request_id = json_object_get_string(id);
    }

    json_object_put(request);
    return valid;
}

[[nodiscard]] bool is_cancel_for(
    const std::string& message,
    const std::string_view expected_id) {
    json_object* cancel = json_tokener_parse(message.c_str());

    if (cancel == nullptr) {
        return false;
    }

    const bool valid =
        json_string_field_equals(cancel, "type", "cancel") &&
        json_string_field_equals(cancel, "id", expected_id);

    json_object_put(cancel);
    return valid;
}

int test_basic_errors() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("basic-errors server failed to start");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        server.shutdown();
        return fail("basic-errors client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"unknown-1","service":"missing.service","operation":"echo","payload":null})")) {
        client.abort();
        server.shutdown();
        return fail("unknown-service request write failed");
    }

    std::string response;

    if (!client.read(response) ||
        !response_has_error(
            response,
            "unknown-1",
            "hub.unknown_service")) {
        client.abort();
        server.shutdown();
        return fail("unknown service did not return hub.unknown_service");
    }

    if (!client.write(
            R"({"type":"request","id":"invalid-1","service":"missing.service","operation":"echo"})")) {
        client.abort();
        server.shutdown();
        return fail("invalid request write failed");
    }

    response.clear();

    if (!client.read(response) ||
        !response_has_error(
            response,
            "invalid-1",
            "hub.invalid_request")) {
        client.abort();
        server.shutdown();
        return fail("invalid request did not return hub.invalid_request");
    }

    client.abort();
    server.shutdown();
    return 0;
}

int test_timeout_and_provider_cancel() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("timeout server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port(), "test.timeout")) {
        provider.abort();
        server.shutdown();
        return fail("timeout provider registration failed");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        provider.abort();
        server.shutdown();
        return fail("timeout client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"timeout-1","service":"test.timeout","operation":"wait","payload":null,"timeout_ms":100})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("timeout request write failed");
    }

    std::string provider_request;

    if (!provider.read(provider_request)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("timeout provider did not receive request");
    }

    std::string provider_request_id;

    if (!extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("timeout provider request is invalid");
    }

    std::string cancel_message;

    if (!provider.read(cancel_message) ||
        !is_cancel_for(cancel_message, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("timeout did not send provider cancel");
    }

    std::string response;

    if (!client.read(response) ||
        !response_has_error(
            response,
            "timeout-1",
            "hub.timeout")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("timeout did not return hub.timeout");
    }

    const std::string late_response =
        R"({"type":"response","id":")" +
        provider_request_id +
        R"(","ok":true,"payload":{"text":"late"}})";

    if (!provider.write(late_response)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("late timeout response write failed");
    }

    if (!client.write(
            R"({"type":"request","id":"after-timeout","service":"test.timeout","operation":"echo","payload":{"text":"alive"},"timeout_ms":1000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("post-timeout request write failed");
    }

    provider_request.clear();

    if (!provider.read(provider_request) ||
        !extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("provider connection did not survive late timeout response");
    }

    const std::string recovery_response =
        R"({"type":"response","id":")" +
        provider_request_id +
        R"(","ok":true,"payload":{"text":"alive"}})";

    if (!provider.write(recovery_response)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("post-timeout provider response write failed");
    }

    response.clear();

    if (!client.read(response) ||
        !response_is_success(
            response,
            "after-timeout",
            "alive")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("provider connection was not usable after late timeout response");
    }

    client.abort();
    provider.abort();
    server.shutdown();
    return 0;
}

int test_client_cancel() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("cancel server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port(), "test.cancel")) {
        provider.abort();
        server.shutdown();
        return fail("cancel provider registration failed");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        provider.abort();
        server.shutdown();
        return fail("cancel client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"cancel-1","service":"test.cancel","operation":"wait","payload":null,"timeout_ms":5000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("cancel request write failed");
    }

    std::string provider_request;

    if (!provider.read(provider_request)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("cancel provider did not receive request");
    }

    std::string provider_request_id;

    if (!extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("cancel provider request is invalid");
    }

    if (!client.write(
            R"({"type":"cancel","id":"cancel-1"})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client cancel write failed");
    }

    std::string provider_cancel;

    if (!provider.read(provider_cancel) ||
        !is_cancel_for(provider_cancel, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client cancel was not forwarded to provider");
    }

    std::string response;

    if (!client.read(response) ||
        !response_has_error(
            response,
            "cancel-1",
            "hub.cancelled")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client cancel did not return hub.cancelled");
    }

    if (!client.write(
            R"({"type":"request","id":"after-cancel","service":"test.cancel","operation":"echo","payload":{"text":"ok"},"timeout_ms":1000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("post-cancel request write failed");
    }

    provider_request.clear();

    if (!provider.read(provider_request) ||
        !extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("provider did not receive post-cancel request");
    }

    const std::string provider_response =
        R"({"type":"response","id":")" +
        provider_request_id +
        R"(","ok":true,"payload":{"text":"ok"}})";

    if (!provider.write(provider_response)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("post-cancel provider response write failed");
    }

    response.clear();

    if (!client.read(response) ||
        !response_is_success(
            response,
            "after-cancel",
            "ok")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client connection was not usable after cancel");
    }

    client.abort();
    provider.abort();
    server.shutdown();
    return 0;
}

int test_provider_disconnect_and_reconnect() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("reconnect server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port(), "test.reconnect")) {
        provider.abort();
        server.shutdown();
        return fail("first reconnect provider registration failed");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        provider.abort();
        server.shutdown();
        return fail("reconnect client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"disconnect-1","service":"test.reconnect","operation":"wait","payload":null,"timeout_ms":5000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("disconnect request write failed");
    }

    std::string provider_request;

    if (!provider.read(provider_request)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("first provider did not receive disconnect request");
    }

    provider.abort();

    std::string response;

    if (!client.read(response) ||
        !response_has_error(
            response,
            "disconnect-1",
            "hub.provider_unavailable")) {
        client.abort();
        server.shutdown();
        return fail("provider disconnect did not return hub.provider_unavailable");
    }

    if (!client.write(
            R"({"type":"request","id":"gap-1","service":"test.reconnect","operation":"echo","payload":null,"timeout_ms":1000})")) {
        client.abort();
        server.shutdown();
        return fail("post-disconnect gap request write failed");
    }

    response.clear();

    if (!client.read(response) ||
        !response_has_error(
            response,
            "gap-1",
            "hub.unknown_service")) {
        client.abort();
        server.shutdown();
        return fail("disconnected provider route was not removed");
    }

    TestWebSocket reconnected_provider;

    if (!register_provider(
            reconnected_provider,
            server.bound_port(),
            "test.reconnect")) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("provider could not re-register after disconnect");
    }

    if (!client.write(
            R"({"type":"request","id":"reconnect-1","service":"test.reconnect","operation":"echo","payload":{"text":"back"},"timeout_ms":1000})")) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("reconnect request write failed");
    }

    provider_request.clear();

    if (!reconnected_provider.read(provider_request)) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("reconnected provider did not receive request");
    }

    std::string provider_request_id;

    if (!extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("reconnected provider request is invalid");
    }

    const std::string provider_response =
        R"({"type":"response","id":")" +
        provider_request_id +
        R"(","ok":true,"payload":{"text":"back"}})";

    if (!reconnected_provider.write(provider_response)) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("reconnected provider response write failed");
    }

    response.clear();

    if (!client.read(response) ||
        !response_is_success(
            response,
            "reconnect-1",
            "back")) {
        client.abort();
        reconnected_provider.abort();
        server.shutdown();
        return fail("request did not recover after provider reconnect");
    }

    client.abort();
    reconnected_provider.abort();
    server.shutdown();
    return 0;
}

int test_client_disconnect_cancels_provider_work() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("client-disconnect server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(
            provider,
            server.bound_port(),
            "test.client-disconnect")) {
        provider.abort();
        server.shutdown();
        return fail("client-disconnect provider registration failed");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        provider.abort();
        server.shutdown();
        return fail("client-disconnect client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"gone-1","service":"test.client-disconnect","operation":"wait","payload":null,"timeout_ms":5000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client-disconnect request write failed");
    }

    std::string provider_request;

    if (!provider.read(provider_request)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("provider did not receive client-disconnect request");
    }

    std::string provider_request_id;

    if (!extract_request_id(provider_request, provider_request_id)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("client-disconnect provider request is invalid");
    }

    client.abort();

    std::string provider_cancel;

    if (!provider.read(provider_cancel) ||
        !is_cancel_for(provider_cancel, provider_request_id)) {
        provider.abort();
        server.shutdown();
        return fail("client disconnect did not cancel provider work");
    }

    provider.abort();
    server.shutdown();
    return 0;
}

int test_bounded_shutdown() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("shutdown server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port(), "test.shutdown")) {
        provider.abort();
        server.shutdown();
        return fail("shutdown provider registration failed");
    }

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port())) {
        provider.abort();
        server.shutdown();
        return fail("shutdown client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"shutdown-1","service":"test.shutdown","operation":"wait","payload":null,"timeout_ms":60000})")) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("shutdown active request write failed");
    }

    std::string provider_request;

    if (!provider.read(provider_request)) {
        client.abort();
        provider.abort();
        server.shutdown();
        return fail("provider did not receive active shutdown request");
    }

    const auto started = std::chrono::steady_clock::now();
    server.shutdown();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);

    client.abort();
    provider.abort();

    if (elapsed > std::chrono::seconds(3)) {
        return fail("Service Hub shutdown exceeded the bounded test limit");
    }

    if (server.running() || server.bound_port() != 0) {
        return fail("Service Hub state was not reset after shutdown");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_basic_errors(); result != 0) {
        return result;
    }

    if (const auto result = test_timeout_and_provider_cancel(); result != 0) {
        return result;
    }

    if (const auto result = test_client_cancel(); result != 0) {
        return result;
    }

    if (const auto result = test_provider_disconnect_and_reconnect();
        result != 0) {
        return result;
    }

    if (const auto result = test_client_disconnect_cancels_provider_work();
        result != 0) {
        return result;
    }

    if (const auto result = test_bounded_shutdown(); result != 0) {
        return result;
    }

    std::cout << "Service Hub lifecycle and error-handling tests passed\n";
    return 0;
}
