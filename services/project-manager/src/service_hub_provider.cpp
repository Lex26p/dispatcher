#include "dispatcher/project_manager/service_hub_provider.hpp"

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

namespace dispatcher::project_manager {
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

[[nodiscard]] json_object* project_json(const Project& project) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, "id", json_object_new_string(project.id.c_str()));
    json_object_object_add(object, "name", json_object_new_string(project.name.c_str()));
    json_object_object_add(
        object,
        "description",
        json_object_new_string(project.description.c_str()));
    return object;
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

[[nodiscard]] std::pair<std::string_view, std::string_view> error_info(
    const ProjectManagerError error) {
    switch (error) {
    case ProjectManagerError::invalid_name:
        return {"project.invalid_name", "Project name must contain a non-whitespace character"};
    case ProjectManagerError::name_too_long:
        return {"project.name_too_long", "Project name exceeds the supported UTF-8 payload size"};
    case ProjectManagerError::description_too_long:
        return {"project.description_too_long", "Project description exceeds the supported UTF-8 payload size"};
    case ProjectManagerError::not_found:
        return {"project.not_found", "Project was not found"};
    case ProjectManagerError::storage_error:
        return {"project.storage_error", "Project storage operation failed"};
    case ProjectManagerError::id_generation_failed:
        return {"project.id_generation_failed", "Project identifier generation failed"};
    case ProjectManagerError::none:
        break;
    }

    return {"project.internal_error", "Project Manager operation failed"};
}

[[nodiscard]] std::string result_response(
    const std::string_view request_id,
    ProjectManagerResult<Project> result) {
    if (!result.ok()) {
        const auto [code, message] = error_info(result.error);
        return error_response(request_id, code, message);
    }

    auto payload = adopt_json(json_object_new_object());
    json_object_object_add(payload.get(), "project", project_json(*result.value));
    return success_response(request_id, payload.release());
}

[[nodiscard]] std::string handle_request(
    ProjectManager& project_manager,
    const std::string_view request_id,
    const std::string_view operation,
    json_object* payload) {
    if (operation == "create-project") {
        if (!object_has_only_fields(payload, {"name", "description"})) {
            return error_response(
                request_id,
                "project.invalid_request",
                "create-project payload must contain only name and optional description");
        }

        std::string name;
        if (!string_field(payload, "name", name)) {
            return error_response(
                request_id,
                "project.invalid_request",
                "create-project requires a string name");
        }

        std::string description;
        json_object* description_value = nullptr;
        if (json_object_object_get_ex(payload, "description", &description_value)) {
            if (description_value == nullptr ||
                !json_object_is_type(description_value, json_type_string)) {
                return error_response(
                    request_id,
                    "project.invalid_request",
                    "create-project description must be a string when present");
            }
            description = json_object_get_string(description_value);
        }

        return result_response(
            request_id,
            project_manager.create(CreateProjectInput{
                .name = std::move(name),
                .description = std::move(description),
            }));
    }

    if (operation == "list-projects") {
        if (!object_has_only_fields(payload, {})) {
            return error_response(
                request_id,
                "project.invalid_request",
                "list-projects payload must be an empty object");
        }

        auto result = project_manager.list();
        if (!result.ok()) {
            const auto [code, message] = error_info(result.error);
            return error_response(request_id, code, message);
        }

        auto payload_object = adopt_json(json_object_new_object());
        json_object* projects = json_object_new_array();
        for (const auto& project : *result.value) {
            json_object_array_add(projects, project_json(project));
        }
        json_object_object_add(payload_object.get(), "projects", projects);
        return success_response(request_id, payload_object.release());
    }

    if (operation == "get-project") {
        if (!object_has_only_fields(payload, {"id"})) {
            return error_response(
                request_id,
                "project.invalid_request",
                "get-project payload must contain only id");
        }

        std::string id;
        if (!string_field(payload, "id", id) || id.empty()) {
            return error_response(
                request_id,
                "project.invalid_request",
                "get-project requires a non-empty string id");
        }

        return result_response(request_id, project_manager.get(id));
    }

    if (operation == "update-project") {
        if (!object_has_only_fields(payload, {"id", "name", "description"})) {
            return error_response(
                request_id,
                "project.invalid_request",
                "update-project payload must contain only id, name and description");
        }

        std::string id;
        std::string name;
        std::string description;
        if (!string_field(payload, "id", id) || id.empty() ||
            !string_field(payload, "name", name) ||
            !string_field(payload, "description", description)) {
            return error_response(
                request_id,
                "project.invalid_request",
                "update-project requires string id, name and description");
        }

        return result_response(
            request_id,
            project_manager.update(UpdateProjectInput{
                .id = std::move(id),
                .name = std::move(name),
                .description = std::move(description),
            }));
    }

    return error_response(
        request_id,
        "project.unknown_operation",
        "Project Manager does not support the requested operation");
}

class HubConnection final {
public:
    HubConnection()
        : resolver_(io_context_),
          websocket_(io_context_) {}

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
    const std::string registration =
        R"({"type":"register","service":"project-manager.v1"})";
    if (!connection.write(registration)) {
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
    ProjectManager& project_manager,
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

    return connection.write(
        handle_request(project_manager, request_id, operation, payload));
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
    ProjectManager& project_manager,
    ServiceHubEndpoint endpoint)
    : project_manager_(project_manager),
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
                healthy = handle_hub_message(connection, project_manager_, message);
                break;
            }
        }

        connection.close();
        if (!stop_requested_.load()) {
            interruptible_sleep(stop_requested_);
        }
    }
}

}  // namespace dispatcher::project_manager
