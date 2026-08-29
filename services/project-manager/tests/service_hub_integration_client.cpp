#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <chrono>
#include <iostream>
#include <memory>
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
using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kService = "project-manager.v1";

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

JsonPtr parse_json(const std::string& text) {
    return JsonPtr(json_tokener_parse(text.c_str()), &json_object_put);
}

bool string_field(json_object* object, const char* name, std::string& value) {
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

    bool connect(const std::string& host, const std::string& port) {
        beast::error_code error;
        auto endpoints = resolver_.resolve(host, port, error);
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

    bool write(std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    bool read(std::string& message, int timeout_ms = 3000) {
        pollfd descriptor{beast::get_lowest_layer(websocket_).native_handle(), POLLIN, 0};
        const int result = ::poll(&descriptor, 1, timeout_ms);
        if (result <= 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 ||
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

std::string request(
    std::string_view id,
    std::string_view operation,
    std::string_view payload_json) {
    return std::string(R"({"type":"request","id":")") + std::string(id) +
           R"(","service":"project-manager.v1","operation":")" +
           std::string(operation) + R"(","payload":)" +
           std::string(payload_json) + R"(,"timeout_ms":5000})";
}

bool is_error(const std::string& text, std::string_view id, std::string_view code) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string response_id;
    if (!string_field(object.get(), "type", type) || type != "response" ||
        !string_field(object.get(), "id", response_id) || response_id != id) return false;
    json_object* ok = nullptr;
    json_object* error = nullptr;
    std::string actual_code;
    return json_object_object_get_ex(object.get(), "ok", &ok) && ok != nullptr &&
           json_object_is_type(ok, json_type_boolean) && json_object_get_boolean(ok) == 0 &&
           json_object_object_get_ex(object.get(), "error", &error) && error != nullptr &&
           string_field(error, "code", actual_code) && actual_code == code;
}

bool extract_project(
    const std::string& text,
    std::string_view expected_id,
    std::string& project_id,
    std::string& name,
    std::string& description) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* project = nullptr;
    if (!string_field(object.get(), "type", type) || type != "response" ||
        !string_field(object.get(), "id", id) || id != expected_id ||
        !json_object_object_get_ex(object.get(), "ok", &ok) || ok == nullptr ||
        json_object_get_boolean(ok) == 0 ||
        !json_object_object_get_ex(object.get(), "payload", &payload) || payload == nullptr ||
        !json_object_object_get_ex(payload, "project", &project) || project == nullptr) {
        return false;
    }
    return string_field(project, "id", project_id) &&
           string_field(project, "name", name) &&
           string_field(project, "description", description);
}

bool list_contains(
    const std::string& text,
    std::string_view expected_id,
    std::string_view expected_name,
    std::string* found_id = nullptr) {
    auto object = parse_json(text);
    if (!object) return false;
    std::string type;
    std::string id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* projects = nullptr;
    if (!string_field(object.get(), "type", type) || type != "response" ||
        !string_field(object.get(), "id", id) || id != expected_id ||
        !json_object_object_get_ex(object.get(), "ok", &ok) || ok == nullptr ||
        json_object_get_boolean(ok) == 0 ||
        !json_object_object_get_ex(object.get(), "payload", &payload) || payload == nullptr ||
        !json_object_object_get_ex(payload, "projects", &projects) || projects == nullptr ||
        !json_object_is_type(projects, json_type_array)) return false;

    const auto count = json_object_array_length(projects);
    for (std::size_t index = 0; index < count; ++index) {
        json_object* project = json_object_array_get_idx(projects, index);
        std::string project_id;
        std::string name;
        if (project != nullptr && string_field(project, "id", project_id) &&
            string_field(project, "name", name) && name == expected_name) {
            if (found_id != nullptr) *found_id = project_id;
            return true;
        }
    }
    return false;
}

bool wait_for_provider(Client& client) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        const std::string id = "probe-" + std::to_string(attempt);
        if (!client.write(request(id, "list-projects", "{}"))) return false;
        std::string response;
        if (!client.read(response)) return false;
        if (!is_error(response, id, "hub.unknown_service")) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

int run_initial(Client& client) {
    if (!client.write(request("unknown-op", "missing-operation", "{}"))) return fail("write unknown operation");
    std::string response;
    if (!client.read(response) || !is_error(response, "unknown-op", "project.unknown_operation")) return fail("unknown operation error");

    if (!client.write(request("bad-payload", "get-project", "{}"))) return fail("write invalid payload");
    if (!client.read(response) || !is_error(response, "bad-payload", "project.invalid_request")) return fail("invalid payload error");

    if (!client.write(request("bad-name", "create-project", R"({"name":"   "})"))) return fail("write invalid name");
    if (!client.read(response) || !is_error(response, "bad-name", "project.invalid_name")) return fail("domain validation error");

    if (!client.write(request("create", "create-project", R"({"name":"Integration Project","description":"created via Service Hub"})"))) return fail("write create");
    if (!client.read(response)) return fail("read create");
    std::string project_id;
    std::string name;
    std::string description;
    if (!extract_project(response, "create", project_id, name, description) ||
        name != "Integration Project" || description != "created via Service Hub") return fail("create response");

    if (!client.write(request("update", "update-project",
            std::string(R"({"id":")") + project_id +
            R"(","name":"Updated Integration Project","description":"updated via Service Hub"})"))) return fail("write update");
    if (!client.read(response)) return fail("read update");
    std::string updated_id;
    if (!extract_project(response, "update", updated_id, name, description) || updated_id != project_id ||
        name != "Updated Integration Project" || description != "updated via Service Hub") return fail("update response");

    if (!client.write(request("parallel-a", "get-project", std::string(R"({"id":")") + project_id + R"("})")) ||
        !client.write(request("parallel-b", "get-project", std::string(R"({"id":")") + project_id + R"("})"))) return fail("write parallel requests");

    std::unordered_map<std::string, bool> seen{{"parallel-a", false}, {"parallel-b", false}};
    for (int index = 0; index < 2; ++index) {
        if (!client.read(response)) return fail("read parallel response");
        for (auto& [id, matched] : seen) {
            std::string response_project_id;
            if (!matched && extract_project(response, id, response_project_id, name, description) && response_project_id == project_id) {
                matched = true;
                break;
            }
        }
    }
    if (!seen["parallel-a"] || !seen["parallel-b"]) return fail("parallel correlation");

    if (!client.write(request("list", "list-projects", "{}")) || !client.read(response) ||
        !list_contains(response, "list", "Updated Integration Project")) return fail("list response");

    return 0;
}

int run_after_reconnect(Client& client) {
    if (!client.write(request("list-after-reconnect", "list-projects", "{}"))) return fail("write list after reconnect");
    std::string response;
    if (!client.read(response)) return fail("read list after reconnect");
    std::string project_id;
    if (!list_contains(response, "list-after-reconnect", "Updated Integration Project", &project_id)) return fail("persisted list after reconnect");

    if (!client.write(request("get-after-reconnect", "get-project", std::string(R"({"id":")") + project_id + R"("})")) ||
        !client.read(response)) return fail("get after reconnect");
    std::string actual_id;
    std::string name;
    std::string description;
    if (!extract_project(response, "get-after-reconnect", actual_id, name, description) || actual_id != project_id ||
        name != "Updated Integration Project") return fail("project after reconnect");
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: service-hub-test-client <host> <port> <initial|after-reconnect>\n";
        return 2;
    }

    Client client;
    if (!client.connect(argv[1], argv[2])) return fail("connect to Service Hub");
    if (!wait_for_provider(client)) return fail("wait for Project Manager provider registration");

    const std::string_view mode = argv[3];
    if (mode == "initial") return run_initial(client);
    if (mode == "after-reconnect") return run_after_reconnect(client);
    return fail("unknown mode");
}
