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
#include <unordered_map>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kBrowserOrigin = "http://127.0.0.1:5173";

int fail(const std::string_view message) {
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
        const int port,
        const bool browser_shaped = false) {
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
                [browser_shaped](websocket::request_type& request) {
                    request.set(
                        http::field::sec_websocket_protocol,
                        kSubprotocol);
                    if (browser_shaped) {
                        request.set(http::field::origin, kBrowserOrigin);
                    }
                }));

        websocket::response_type response;
        websocket_.handshake(
            response,
            host + ":" + std::to_string(port),
            "/v1/ws",
            error);

        if (error) {
            return false;
        }

        const auto negotiated = response[http::field::sec_websocket_protocol];
        return std::string_view(negotiated.data(), negotiated.size()) ==
               kSubprotocol;
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

        const int poll_result = ::poll(&descriptor, 1, timeout_ms);

        if (poll_result <= 0 ||
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

[[nodiscard]] bool string_field_equals(
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

[[nodiscard]] bool register_provider(
    TestWebSocket& provider,
    const int port,
    const std::string_view service) {
    if (!provider.connect("127.0.0.1", port)) {
        return false;
    }

    const std::string message =
        R"({"type":"register","service":")" +
        std::string(service) +
        R"("})";

    if (!provider.write(message)) {
        return false;
    }

    std::string response_text;
    if (!provider.read(response_text)) {
        return false;
    }

    json_object* response = json_tokener_parse(response_text.c_str());
    if (response == nullptr) {
        return false;
    }

    const bool valid =
        string_field_equals(response, "type", "registered") &&
        string_field_equals(response, "service", service);

    json_object_put(response);
    return valid;
}

[[nodiscard]] bool extract_provider_request(
    const std::string& message,
    std::string& id,
    std::string& text) {
    json_object* request = json_tokener_parse(message.c_str());
    if (request == nullptr) {
        return false;
    }

    json_object* id_value = nullptr;
    json_object* payload = nullptr;
    json_object* text_value = nullptr;

    const bool valid =
        string_field_equals(request, "type", "request") &&
        string_field_equals(request, "service", "test.acceptance") &&
        string_field_equals(request, "operation", "echo") &&
        json_object_object_get_ex(request, "id", &id_value) &&
        json_object_is_type(id_value, json_type_string) &&
        json_object_object_get_ex(request, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "text", &text_value) &&
        json_object_is_type(text_value, json_type_string);

    if (valid) {
        id = json_object_get_string(id_value);
        text = json_object_get_string(text_value);
    }

    json_object_put(request);
    return valid;
}

[[nodiscard]] bool send_success(
    TestWebSocket& provider,
    const std::string_view provider_id,
    const std::string_view text) {
    const std::string response =
        R"({"type":"response","id":")" +
        std::string(provider_id) +
        R"(","ok":true,"payload":{"text":")" +
        std::string(text) +
        R"("}})";
    return provider.write(response);
}

[[nodiscard]] bool response_success_text(
    const std::string& message,
    std::string& id,
    std::string& text) {
    json_object* response = json_tokener_parse(message.c_str());
    if (response == nullptr) {
        return false;
    }

    json_object* id_value = nullptr;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* text_value = nullptr;

    const bool valid =
        string_field_equals(response, "type", "response") &&
        json_object_object_get_ex(response, "id", &id_value) &&
        json_object_is_type(id_value, json_type_string) &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) != 0 &&
        json_object_object_get_ex(response, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "text", &text_value) &&
        json_object_is_type(text_value, json_type_string);

    if (valid) {
        id = json_object_get_string(id_value);
        text = json_object_get_string(text_value);
    }

    json_object_put(response);
    return valid;
}

[[nodiscard]] bool response_error_code(
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
        string_field_equals(response, "type", "response") &&
        string_field_equals(response, "id", expected_id) &&
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

[[nodiscard]] bool write_request(
    TestWebSocket& client,
    const std::string_view id,
    const std::string_view service,
    const std::string_view text,
    const int timeout_ms = 3000) {
    const std::string request =
        R"({"type":"request","id":")" + std::string(id) +
        R"(","service":")" + std::string(service) +
        R"(","operation":"echo","payload":{"text":")" +
        std::string(text) +
        R"("},"timeout_ms":)" + std::to_string(timeout_ms) + "}";
    return client.write(request);
}

int run_acceptance() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");
    if (!server.start() || server.bound_port() <= 0) {
        return fail("Service Hub failed to start");
    }

    const int port = server.bound_port();
    TestWebSocket provider;
    if (!register_provider(provider, port, "test.acceptance")) {
        server.shutdown();
        return fail("test provider failed to register");
    }

    TestWebSocket browser_client;
    if (!browser_client.connect("127.0.0.1", port, true)) {
        provider.abort();
        server.shutdown();
        return fail("browser-shaped client boundary failed");
    }

    if (!write_request(
            browser_client,
            "basic-1",
            "test.acceptance",
            "basic")) {
        return fail("basic client request failed to send");
    }

    std::string provider_message;
    std::string provider_id;
    std::string provider_text;
    if (!provider.read(provider_message) ||
        !extract_provider_request(provider_message, provider_id, provider_text) ||
        provider_text != "basic" ||
        !send_success(provider, provider_id, "basic-ok")) {
        return fail("basic provider route failed");
    }

    std::string client_message;
    std::string client_id;
    std::string client_text;
    if (!browser_client.read(client_message) ||
        !response_success_text(client_message, client_id, client_text) ||
        client_id != "basic-1" || client_text != "basic-ok") {
        return fail("basic response did not return to browser-shaped client");
    }

    if (!write_request(
            browser_client,
            "parallel-1",
            "test.acceptance",
            "first") ||
        !write_request(
            browser_client,
            "parallel-2",
            "test.acceptance",
            "second")) {
        return fail("parallel requests failed to send");
    }

    std::unordered_map<std::string, std::string> provider_ids;
    for (int index = 0; index < 2; ++index) {
        provider_message.clear();
        provider_id.clear();
        provider_text.clear();
        if (!provider.read(provider_message) ||
            !extract_provider_request(provider_message, provider_id, provider_text)) {
            return fail("provider did not receive both parallel requests");
        }
        provider_ids[provider_text] = provider_id;
    }

    if (!provider_ids.contains("first") ||
        !provider_ids.contains("second") ||
        provider_ids["first"] == provider_ids["second"] ||
        !send_success(provider, provider_ids["second"], "second-ok") ||
        !send_success(provider, provider_ids["first"], "first-ok")) {
        return fail("parallel provider correlation setup failed");
    }

    std::unordered_map<std::string, std::string> client_results;
    for (int index = 0; index < 2; ++index) {
        client_message.clear();
        client_id.clear();
        client_text.clear();
        if (!browser_client.read(client_message) ||
            !response_success_text(client_message, client_id, client_text)) {
            return fail("parallel client response is invalid");
        }
        client_results[client_id] = client_text;
    }

    if (client_results["parallel-1"] != "first-ok" ||
        client_results["parallel-2"] != "second-ok") {
        return fail("parallel responses were mixed");
    }

    if (!write_request(
            browser_client,
            "unknown-1",
            "missing.service",
            "missing")) {
        return fail("unknown-service request failed to send");
    }

    client_message.clear();
    if (!browser_client.read(client_message) ||
        !response_error_code(
            client_message,
            "unknown-1",
            "hub.unknown_service")) {
        return fail("unknown service did not return hub.unknown_service");
    }

    if (!write_request(
            browser_client,
            "disconnect-1",
            "test.acceptance",
            "disconnect",
            5000)) {
        return fail("disconnect scenario request failed to send");
    }

    provider_message.clear();
    if (!provider.read(provider_message) ||
        !extract_provider_request(provider_message, provider_id, provider_text)) {
        return fail("provider did not receive disconnect scenario request");
    }

    provider.abort();

    client_message.clear();
    if (!browser_client.read(client_message) ||
        !response_error_code(
            client_message,
            "disconnect-1",
            "hub.provider_unavailable")) {
        browser_client.abort();
        server.shutdown();
        return fail("provider disconnect did not return hub.provider_unavailable");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    TestWebSocket reconnected_provider;
    if (!register_provider(
            reconnected_provider,
            port,
            "test.acceptance")) {
        browser_client.abort();
        server.shutdown();
        return fail("provider could not re-register after disconnect");
    }

    if (!write_request(
            browser_client,
            "reconnect-1",
            "test.acceptance",
            "reconnected")) {
        reconnected_provider.abort();
        browser_client.abort();
        server.shutdown();
        return fail("post-reconnect request failed to send");
    }

    provider_message.clear();
    provider_id.clear();
    provider_text.clear();
    if (!reconnected_provider.read(provider_message) ||
        !extract_provider_request(provider_message, provider_id, provider_text) ||
        provider_text != "reconnected" ||
        !send_success(reconnected_provider, provider_id, "reconnected-ok")) {
        reconnected_provider.abort();
        browser_client.abort();
        server.shutdown();
        return fail("post-reconnect provider route failed");
    }

    client_message.clear();
    client_id.clear();
    client_text.clear();
    if (!browser_client.read(client_message) ||
        !response_success_text(client_message, client_id, client_text) ||
        client_id != "reconnect-1" ||
        client_text != "reconnected-ok") {
        reconnected_provider.abort();
        browser_client.abort();
        server.shutdown();
        return fail("post-reconnect response failed");
    }

    if (!write_request(
            browser_client,
            "shutdown-1",
            "test.acceptance",
            "long-running",
            5000)) {
        reconnected_provider.abort();
        browser_client.abort();
        server.shutdown();
        return fail("shutdown scenario request failed to send");
    }

    provider_message.clear();
    if (!reconnected_provider.read(provider_message)) {
        reconnected_provider.abort();
        browser_client.abort();
        server.shutdown();
        return fail("shutdown scenario did not reach provider");
    }

    const auto shutdown_started = std::chrono::steady_clock::now();
    server.shutdown();
    const auto shutdown_elapsed =
        std::chrono::steady_clock::now() - shutdown_started;

    browser_client.abort();
    reconnected_provider.abort();

    if (shutdown_elapsed > std::chrono::seconds(4)) {
        return fail("Service Hub shutdown was not bounded");
    }

    if (server.running() || server.bound_port() != 0) {
        return fail("Service Hub did not finish shutdown cleanly");
    }

    std::cout << "CORE-002 Service Hub acceptance scenario passed\n";
    return 0;
}

}  // namespace

int main() {
    return run_acceptance();
}
