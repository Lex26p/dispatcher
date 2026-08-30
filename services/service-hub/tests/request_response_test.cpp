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
#include <unordered_map>

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
        websocket_.close(websocket::close_code::normal, error);
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

[[nodiscard]] bool register_provider(
    TestWebSocket& provider,
    const int port) {
    if (!provider.connect("127.0.0.1", port) ||
        !provider.write(
            R"({"type":"register","service":"test.echo"})")) {
        return false;
    }

    std::string registered_message;

    if (!provider.read(registered_message)) {
        return false;
    }

    json_object* registered =
        json_tokener_parse(registered_message.c_str());

    const bool valid =
        registered != nullptr &&
        json_string_field_equals(registered, "type", "registered") &&
        json_string_field_equals(registered, "service", "test.echo");

    if (registered != nullptr) {
        json_object_put(registered);
    }

    return valid;
}

[[nodiscard]] bool parse_provider_request(
    const std::string& message,
    std::string& hub_id,
    std::string& marker) {
    json_object* request = json_tokener_parse(message.c_str());

    if (request == nullptr ||
        !json_string_field_equals(request, "type", "request") ||
        !json_string_field_equals(request, "service", "test.echo") ||
        !json_string_field_equals(request, "operation", "echo")) {
        if (request != nullptr) {
            json_object_put(request);
        }
        return false;
    }

    json_object* id = nullptr;
    json_object* payload = nullptr;
    json_object* marker_value = nullptr;

    const bool valid =
        json_object_object_get_ex(request, "id", &id) &&
        json_object_is_type(id, json_type_string) &&
        std::string_view(json_object_get_string(id)).find("hub-") == 0 &&
        json_object_object_get_ex(request, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "marker", &marker_value) &&
        json_object_is_type(marker_value, json_type_string);

    if (valid) {
        hub_id = json_object_get_string(id);
        marker = json_object_get_string(marker_value);
    }

    json_object_put(request);
    return valid;
}

[[nodiscard]] bool provider_request_has_session_auth(
    const std::string& message,
    const std::string_view expected_token) {
    json_object* request = json_tokener_parse(message.c_str());
    if (request == nullptr) {
        return false;
    }

    json_object* auth = nullptr;
    json_object* type = nullptr;
    json_object* token = nullptr;
    const bool valid =
        json_object_object_get_ex(request, "auth", &auth) &&
        json_object_is_type(auth, json_type_object) &&
        json_object_object_get_ex(auth, "type", &type) &&
        json_object_is_type(type, json_type_string) &&
        std::string_view(json_object_get_string(type)) == "session" &&
        json_object_object_get_ex(auth, "token", &token) &&
        json_object_is_type(token, json_type_string) &&
        std::string_view(json_object_get_string(token)) == expected_token;

    json_object_put(request);
    return valid;
}

[[nodiscard]] bool parse_client_success(
    const std::string& message,
    std::string& request_id,
    std::string& marker) {
    json_object* response = json_tokener_parse(message.c_str());

    if (response == nullptr ||
        !json_string_field_equals(response, "type", "response")) {
        if (response != nullptr) {
            json_object_put(response);
        }
        return false;
    }

    json_object* id = nullptr;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* marker_value = nullptr;

    const bool valid =
        json_object_object_get_ex(response, "id", &id) &&
        json_object_is_type(id, json_type_string) &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) != 0 &&
        json_object_object_get_ex(response, "payload", &payload) &&
        json_object_is_type(payload, json_type_object) &&
        json_object_object_get_ex(payload, "marker", &marker_value) &&
        json_object_is_type(marker_value, json_type_string);

    if (valid) {
        request_id = json_object_get_string(id);
        marker = json_object_get_string(marker_value);
    }

    json_object_put(response);
    return valid;
}

[[nodiscard]] bool parse_client_error(
    const std::string& message,
    std::string& request_id,
    std::string& error_code) {
    json_object* response = json_tokener_parse(message.c_str());

    if (response == nullptr ||
        !json_string_field_equals(response, "type", "response")) {
        if (response != nullptr) {
            json_object_put(response);
        }
        return false;
    }

    json_object* id = nullptr;
    json_object* ok = nullptr;
    json_object* error = nullptr;
    json_object* code = nullptr;

    const bool valid =
        json_object_object_get_ex(response, "id", &id) &&
        json_object_is_type(id, json_type_string) &&
        json_object_object_get_ex(response, "ok", &ok) &&
        json_object_is_type(ok, json_type_boolean) &&
        json_object_get_boolean(ok) == 0 &&
        json_object_object_get_ex(response, "error", &error) &&
        json_object_is_type(error, json_type_object) &&
        json_object_object_get_ex(error, "code", &code) &&
        json_object_is_type(code, json_type_string);

    if (valid) {
        request_id = json_object_get_string(id);
        error_code = json_object_get_string(code);
    }

    json_object_put(response);
    return valid;
}

[[nodiscard]] std::string make_provider_response(
    const std::string& hub_id,
    const std::string& marker) {
    return R"({"type":"response","id":")" +
           hub_id +
           R"(","ok":true,"payload":{"marker":")" +
           marker +
           R"("}})";
}

int test_request_response_route() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("Service Hub server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port())) {
        server.shutdown();
        return fail("provider registration failed");
    }

    std::atomic<bool> provider_ok{false};

    std::thread provider_thread([&provider, &provider_ok] {
        std::string request_message;
        std::string hub_id;
        std::string marker;

        if (!provider.read(request_message) ||
            !parse_provider_request(request_message, hub_id, marker) ||
            marker != "hello" ||
            !provider.write(make_provider_response(hub_id, marker))) {
            return;
        }

        provider_ok.store(true);
    });

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port()) ||
        !client.write(
            R"({"type":"request","id":"req-42","service":"test.echo","operation":"echo","payload":{"marker":"hello"},"timeout_ms":5000})")) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("client request failed");
    }

    std::string response_message;
    std::string request_id;
    std::string marker;

    const bool response_valid =
        client.read(response_message) &&
        parse_client_success(response_message, request_id, marker) &&
        request_id == "req-42" &&
        marker == "hello";

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

int test_parallel_requests_on_one_client() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("parallel test server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port())) {
        server.shutdown();
        return fail("parallel test provider registration failed");
    }

    std::atomic<bool> provider_ok{false};

    std::thread provider_thread([&provider, &provider_ok] {
        std::unordered_map<std::string, std::string> ids_by_marker;

        for (int index = 0; index < 2; ++index) {
            std::string message;
            std::string hub_id;
            std::string marker;

            if (!provider.read(message) ||
                !parse_provider_request(message, hub_id, marker)) {
                return;
            }

            ids_by_marker.emplace(marker, hub_id);
        }

        if (!ids_by_marker.contains("slow") ||
            !ids_by_marker.contains("fast") ||
            ids_by_marker.at("slow") == ids_by_marker.at("fast")) {
            return;
        }

        if (!provider.write(make_provider_response(
                ids_by_marker.at("fast"),
                "fast")) ||
            !provider.write(make_provider_response(
                ids_by_marker.at("slow"),
                "slow"))) {
            return;
        }

        provider_ok.store(true);
    });

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port()) ||
        !client.write(
            R"({"type":"request","id":"req-slow","service":"test.echo","operation":"echo","payload":{"marker":"slow"},"timeout_ms":5000})") ||
        !client.write(
            R"({"type":"request","id":"req-fast","service":"test.echo","operation":"echo","payload":{"marker":"fast"},"timeout_ms":5000})")) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("parallel client writes failed");
    }

    std::string first_response;
    std::string second_response;
    std::string first_id;
    std::string first_marker;
    std::string second_id;
    std::string second_marker;

    const bool responses_valid =
        client.read(first_response) &&
        parse_client_success(first_response, first_id, first_marker) &&
        client.read(second_response) &&
        parse_client_success(second_response, second_id, second_marker) &&
        first_id == "req-fast" &&
        first_marker == "fast" &&
        second_id == "req-slow" &&
        second_marker == "slow";

    client.close();
    provider_thread.join();
    provider.close();
    server.shutdown();

    if (!provider_ok.load()) {
        return fail("provider did not process both parallel requests");
    }

    if (!responses_valid) {
        return fail("parallel responses were not correlated out of order");
    }

    return 0;
}

int test_slow_request_does_not_block_fast_request() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("slow/fast test server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port())) {
        server.shutdown();
        return fail("slow/fast provider registration failed");
    }

    std::atomic<bool> provider_ok{false};

    std::thread provider_thread([&provider, &provider_ok] {
        std::unordered_map<std::string, std::string> ids_by_marker;

        for (int index = 0; index < 2; ++index) {
            std::string message;
            std::string hub_id;
            std::string marker_value;

            if (!provider.read(message) ||
                !parse_provider_request(message, hub_id, marker_value)) {
                return;
            }

            ids_by_marker.emplace(marker_value, hub_id);
        }

        if (!ids_by_marker.contains("slow-timeout") ||
            !ids_by_marker.contains("fast-success") ||
            !provider.write(make_provider_response(
                ids_by_marker.at("fast-success"),
                "fast-success"))) {
            return;
        }

        provider_ok.store(true);
    });

    TestWebSocket client;

    if (!client.connect("127.0.0.1", server.bound_port()) ||
        !client.write(
            R"({"type":"request","id":"req-timeout","service":"test.echo","operation":"echo","payload":{"marker":"slow-timeout"},"timeout_ms":100})") ||
        !client.write(
            R"({"type":"request","id":"req-fast","service":"test.echo","operation":"echo","payload":{"marker":"fast-success"},"timeout_ms":5000})")) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("slow/fast client writes failed");
    }

    std::string first_response;
    std::string first_id;
    std::string first_marker;
    std::string second_response;
    std::string second_id;
    std::string second_error;

    const bool responses_valid =
        client.read(first_response) &&
        parse_client_success(first_response, first_id, first_marker) &&
        first_id == "req-fast" &&
        first_marker == "fast-success" &&
        client.read(second_response) &&
        parse_client_error(second_response, second_id, second_error) &&
        second_id == "req-timeout" &&
        second_error == "hub.timeout";

    client.close();
    provider_thread.join();
    provider.close();
    server.shutdown();

    if (!provider_ok.load()) {
        return fail("provider did not receive both slow/fast requests");
    }

    if (!responses_valid) {
        return fail("slow request blocked or corrupted fast request correlation");
    }

    return 0;
}

int test_same_request_id_on_different_clients() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");

    if (!server.start()) {
        return fail("multi-client test server failed to start");
    }

    TestWebSocket provider;

    if (!register_provider(provider, server.bound_port())) {
        server.shutdown();
        return fail("multi-client provider registration failed");
    }

    std::atomic<bool> provider_ok{false};

    std::thread provider_thread([&provider, &provider_ok] {
        std::unordered_map<std::string, std::string> ids_by_marker;

        for (int index = 0; index < 2; ++index) {
            std::string message;
            std::string hub_id;
            std::string marker;

            if (!provider.read(message) ||
                !parse_provider_request(message, hub_id, marker)) {
                return;
            }

            ids_by_marker.emplace(marker, hub_id);
        }

        if (!ids_by_marker.contains("client-a") ||
            !ids_by_marker.contains("client-b") ||
            ids_by_marker.at("client-a") == ids_by_marker.at("client-b")) {
            return;
        }

        if (!provider.write(make_provider_response(
                ids_by_marker.at("client-b"),
                "client-b")) ||
            !provider.write(make_provider_response(
                ids_by_marker.at("client-a"),
                "client-a"))) {
            return;
        }

        provider_ok.store(true);
    });

    TestWebSocket client_a;
    TestWebSocket client_b;

    if (!client_a.connect("127.0.0.1", server.bound_port()) ||
        !client_b.connect("127.0.0.1", server.bound_port()) ||
        !client_a.write(
            R"({"type":"request","id":"same-id","service":"test.echo","operation":"echo","payload":{"marker":"client-a"},"timeout_ms":5000})") ||
        !client_b.write(
            R"({"type":"request","id":"same-id","service":"test.echo","operation":"echo","payload":{"marker":"client-b"},"timeout_ms":5000})")) {
        client_a.close();
        client_b.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("multi-client request writes failed");
    }

    std::string response_a;
    std::string response_b;
    std::string id_a;
    std::string marker_a;
    std::string id_b;
    std::string marker_b;

    const bool responses_valid =
        client_a.read(response_a) &&
        parse_client_success(response_a, id_a, marker_a) &&
        client_b.read(response_b) &&
        parse_client_success(response_b, id_b, marker_b) &&
        id_a == "same-id" &&
        marker_a == "client-a" &&
        id_b == "same-id" &&
        marker_b == "client-b";

    client_a.close();
    client_b.close();
    provider_thread.join();
    provider.close();
    server.shutdown();

    if (!provider_ok.load()) {
        return fail("provider did not receive distinct Hub IDs for two clients");
    }

    if (!responses_valid) {
        return fail("same client request IDs conflicted across connections");
    }

    return 0;
}

int test_session_authentication_is_forwarded_and_malformed_rejected() {
    dispatcher::service_hub::ServiceHubServer server("127.0.0.1:0");
    if (!server.start()) {
        return fail("auth forwarding test server failed to start");
    }

    TestWebSocket provider;
    if (!register_provider(provider, server.bound_port())) {
        server.shutdown();
        return fail("auth forwarding provider registration failed");
    }

    const std::string token(64, 'a');
    std::atomic<bool> provider_ok{false};
    std::thread provider_thread([&provider, &provider_ok, &token] {
        std::string message;
        std::string hub_id;
        std::string marker;
        if (!provider.read(message) ||
            !parse_provider_request(message, hub_id, marker) ||
            marker != "authenticated" ||
            !provider_request_has_session_auth(message, token) ||
            !provider.write(make_provider_response(hub_id, marker))) {
            return;
        }
        provider_ok.store(true);
    });

    TestWebSocket client;
    const std::string authenticated_request =
        R"({"type":"request","id":"req-auth","service":"test.echo","operation":"echo","payload":{"marker":"authenticated"},"auth":{"type":"session","token":")" +
        token +
        R"("},"timeout_ms":5000})";

    if (!client.connect("127.0.0.1", server.bound_port()) ||
        !client.write(authenticated_request)) {
        client.close();
        provider.close();
        provider_thread.join();
        server.shutdown();
        return fail("authenticated client request failed");
    }

    std::string response;
    std::string response_id;
    std::string marker;
    const bool authenticated_response =
        client.read(response) &&
        parse_client_success(response, response_id, marker) &&
        response_id == "req-auth" &&
        marker == "authenticated";

    provider_thread.join();

    if (!client.write(
            R"({"type":"request","id":"req-bad-auth","service":"test.echo","operation":"echo","payload":{"marker":"bad"},"auth":{"type":"session","token":"abc"}})")) {
        client.close();
        provider.close();
        server.shutdown();
        return fail("malformed authentication request write failed");
    }

    std::string error_response;
    std::string error_id;
    std::string error_code;
    const bool malformed_rejected =
        client.read(error_response) &&
        parse_client_error(error_response, error_id, error_code) &&
        error_id == "req-bad-auth" &&
        error_code == "hub.invalid_request";

    client.close();
    provider.close();
    server.shutdown();

    if (!provider_ok.load() || !authenticated_response) {
        return fail("session authentication was not forwarded to the provider");
    }
    if (!malformed_rejected) {
        return fail("malformed session authentication was not rejected by the Hub");
    }
    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_request_response_route(); result != 0) {
        return result;
    }

    if (const auto result = test_parallel_requests_on_one_client(); result != 0) {
        return result;
    }

    if (const auto result = test_slow_request_does_not_block_fast_request(); result != 0) {
        return result;
    }

    if (const auto result = test_same_request_id_on_different_clients(); result != 0) {
        return result;
    }

    if (const auto result = test_session_authentication_is_forwarded_and_malformed_rejected(); result != 0) {
        return result;
    }

    std::cout << "Service Hub parallel correlation/auth forwarding tests passed\n";
    return 0;
}
