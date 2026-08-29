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
constexpr std::string_view kBrowserOrigin = "http://127.0.0.1:5173";

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

class TestWebSocket final {
public:
    TestWebSocket() : resolver_(io_context_), websocket_(io_context_) {}

    [[nodiscard]] bool connect(const std::string& host, int port, bool browser_shaped = false) {
        beast::error_code error;
        const auto endpoints = resolver_.resolve(host, std::to_string(port), error);
        if (error) return false;
        asio::connect(beast::get_lowest_layer(websocket_), endpoints, error);
        if (error) return false;

        websocket_.set_option(websocket::stream_base::decorator(
            [browser_shaped](websocket::request_type& request) {
                request.set(http::field::sec_websocket_protocol, kSubprotocol);
                if (browser_shaped) {
                    request.set(http::field::origin, kBrowserOrigin);
                }
            }));

        websocket::response_type response;
        websocket_.handshake(response, host + ":" + std::to_string(port), "/v1/ws", error);
        if (error) return false;

        const auto selected = response[http::field::sec_websocket_protocol];
        negotiated_subprotocol_.assign(selected.data(), selected.size());
        return true;
    }

    [[nodiscard]] bool write(std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(std::string& message) {
        beast::flat_buffer buffer;
        beast::error_code error;
        websocket_.read(buffer, error);
        if (error) return false;
        message = beast::buffers_to_string(buffer.data());
        return websocket_.got_text();
    }

    [[nodiscard]] std::string_view negotiated_subprotocol() const noexcept {
        return negotiated_subprotocol_;
    }

    void close() {
        beast::error_code error;
        websocket_.close(websocket::close_code::normal, error);
    }

private:
    asio::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> websocket_;
    std::string negotiated_subprotocol_;
};

bool json_string_field_equals(json_object* object, const char* field, std::string_view expected) {
    json_object* value = nullptr;
    return json_object_object_get_ex(object, field, &value) &&
           json_object_is_type(value, json_type_string) &&
           std::string_view(json_object_get_string(value)) == expected;
}

int test_browser_boundary() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");
    if (!server.start()) return fail("Service Hub server failed to start");
    const int port = server.bound_port();

    TestWebSocket provider;
    if (!provider.connect("127.0.0.1", port)) {
        server.shutdown();
        return fail("provider failed to connect");
    }
    if (!provider.write(R"({"type":"register","service":"test.browser"})")) {
        provider.close(); server.shutdown(); return fail("provider registration write failed");
    }
    std::string registered;
    if (!provider.read(registered)) {
        provider.close(); server.shutdown(); return fail("provider registration response missing");
    }

    std::atomic<bool> provider_ok{false};
    std::thread provider_thread([&] {
        std::string request_message;
        if (!provider.read(request_message)) return;
        json_object* request = json_tokener_parse(request_message.c_str());
        if (request == nullptr ||
            !json_string_field_equals(request, "type", "request") ||
            !json_string_field_equals(request, "service", "test.browser") ||
            !json_string_field_equals(request, "operation", "ping")) {
            if (request != nullptr) json_object_put(request);
            return;
        }
        json_object* id = nullptr;
        if (!json_object_object_get_ex(request, "id", &id) ||
            !json_object_is_type(id, json_type_string)) {
            json_object_put(request);
            return;
        }
        const std::string hub_id = json_object_get_string(id);
        json_object_put(request);
        const std::string response =
            R"({"type":"response","id":")" + hub_id +
            R"(","ok":true,"payload":{"browserBoundary":true}})";
        provider_ok.store(provider.write(response));
    });

    TestWebSocket browser;
    if (!browser.connect("127.0.0.1", port, true)) {
        provider.close(); provider_thread.join(); server.shutdown();
        return fail("browser-shaped WebSocket handshake failed");
    }
    if (browser.negotiated_subprotocol() != kSubprotocol) {
        browser.close(); provider.close(); provider_thread.join(); server.shutdown();
        return fail("Service Hub did not negotiate the v1 WebSocket subprotocol");
    }
    if (!browser.write(R"({"type":"request","id":"web-1","service":"test.browser","operation":"ping","payload":null,"timeout_ms":5000})")) {
        browser.close(); provider.close(); provider_thread.join(); server.shutdown();
        return fail("browser-shaped client request write failed");
    }
    std::string response_message;
    if (!browser.read(response_message)) {
        browser.close(); provider.close(); provider_thread.join(); server.shutdown();
        return fail("browser-shaped client response missing");
    }

    json_object* response = json_tokener_parse(response_message.c_str());
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* boundary = nullptr;
    const bool valid = response != nullptr &&
        json_string_field_equals(response, "type", "response") &&
        json_string_field_equals(response, "id", "web-1") &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_get_boolean(ok) != 0 &&
        json_object_object_get_ex(response, "payload", &payload) &&
        json_object_object_get_ex(payload, "browserBoundary", &boundary) &&
        json_object_get_boolean(boundary) != 0;
    if (response != nullptr) json_object_put(response);

    browser.close();
    provider_thread.join();
    provider.close();
    server.shutdown();

    if (!provider_ok.load()) return fail("provider did not answer browser-shaped request");
    if (!valid) return fail("browser-shaped client received unexpected response");
    return 0;
}
}  // namespace

int main() {
    if (const auto result = test_browser_boundary(); result != 0) return result;
    std::cout << "Service Hub browser boundary test passed\n";
    return 0;
}
