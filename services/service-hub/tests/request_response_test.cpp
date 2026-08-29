#include "dispatcher/service_hub/server.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <atomic>
#include <iostream>
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

        if (error) {
            return false;
        }

        return true;
    }

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(std::string& message) {
        beast::flat_buffer buffer;
        beast::error_code error;

        websocket_.read(buffer, error);

        if (error) {
            return false;
        }

        message = beast::buffers_to_string(buffer.data());
        return websocket_.got_text();
    }

    void close() {
        beast::error_code error;
        websocket_.close(
            websocket::close_code::normal,
            error);
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

int test_request_response_route() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("Service Hub server failed to start");
    }

    const int port = server.bound_port();

    if (port <= 0) {
        server.shutdown();
        return fail("Service Hub did not expose a bound port");
    }

    TestWebSocket provider;

    if (!provider.connect("127.0.0.1", port)) {
        server.shutdown();
        return fail("provider failed to connect");
    }

    if (!provider.write(
            R"({"type":"register","service":"test.echo"})")) {
        provider.close();
        server.shutdown();
        return fail("provider registration write failed");
    }

    std::string registered_message;

    if (!provider.read(registered_message)) {
        provider.close();
        server.shutdown();
        return fail("provider registration response was not received");
    }

    json_object* registered =
        json_tokener_parse(registered_message.c_str());

    if (registered == nullptr ||
        !json_string_field_equals(
            registered,
            "type",
            "registered") ||
        !json_string_field_equals(
            registered,
            "service",
            "test.echo")) {
        if (registered != nullptr) {
            json_object_put(registered);
        }
        provider.close();
        server.shutdown();
        return fail("provider registration response is invalid");
    }

    json_object_put(registered);

    std::atomic<bool> provider_ok{false};

    std::thread provider_thread([&provider, &provider_ok] {
        std::string request_message;

        if (!provider.read(request_message)) {
            return;
        }

        json_object* request =
            json_tokener_parse(request_message.c_str());

        if (request == nullptr ||
            !json_string_field_equals(
                request,
                "type",
                "request") ||
            !json_string_field_equals(
                request,
                "service",
                "test.echo") ||
            !json_string_field_equals(
                request,
                "operation",
                "echo")) {
            if (request != nullptr) {
                json_object_put(request);
            }
            return;
        }

        json_object* id = nullptr;
        json_object* payload = nullptr;
        json_object* text = nullptr;

        if (!json_object_object_get_ex(request, "id", &id) ||
            !json_object_is_type(id, json_type_string) ||
            std::string_view(json_object_get_string(id)).find("hub-") != 0 ||
            !json_object_object_get_ex(request, "payload", &payload) ||
            !json_object_is_type(payload, json_type_object) ||
            !json_object_object_get_ex(payload, "text", &text) ||
            !json_object_is_type(text, json_type_string) ||
            std::string_view(json_object_get_string(text)) != "hello") {
            json_object_put(request);
            return;
        }

        const std::string provider_request_id =
            json_object_get_string(id);

        json_object_put(request);

        const std::string response =
            R"({"type":"response","id":")" +
            provider_request_id +
            R"(","ok":true,"payload":{"text":"hello","provider":"test.echo"}})";

        if (!provider.write(response)) {
            return;
        }

        provider_ok.store(true);
    });

    TestWebSocket client;

    if (!client.connect("127.0.0.1", port)) {
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("client failed to connect");
    }

    if (!client.write(
            R"({"type":"request","id":"req-42","service":"test.echo","operation":"echo","payload":{"text":"hello"},"timeout_ms":5000})")) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("client request write failed");
    }

    std::string response_message;

    if (!client.read(response_message)) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("client response was not received");
    }

    json_object* response =
        json_tokener_parse(response_message.c_str());

    if (response == nullptr) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("client response is not valid JSON");
    }

    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* text = nullptr;
    json_object* provider_name = nullptr;

    const bool response_valid =
        json_string_field_equals(response, "type", "response") &&
        json_string_field_equals(response, "id", "req-42") &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) != 0 &&
        json_object_object_get_ex(response, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "text", &text) &&
        json_object_is_type(text, json_type_string) &&
        std::string_view(json_object_get_string(text)) == "hello" &&
        json_object_object_get_ex(
            payload,
            "provider",
            &provider_name) &&
        json_object_is_type(provider_name, json_type_string) &&
        std::string_view(
            json_object_get_string(provider_name)) == "test.echo";

    json_object_put(response);

    client.close();
    provider_thread.join();
    provider.close();
    server.shutdown();

    if (!provider_ok.load()) {
        return fail("provider did not receive and answer the routed request");
    }

    if (!response_valid) {
        return fail("client received an unexpected routed response");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_request_response_route(); result != 0) {
        return result;
    }

    std::cout << "Service Hub WebSocket request/response test passed\n";
    return 0;
}
