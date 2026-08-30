#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

constexpr std::string_view subprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view endpoint = "/v1/ws";
constexpr std::string_view service = "users-access.v1";
constexpr std::string_view user_password = "Step6A operator password";
constexpr std::string_view replacement_password = "Step6A replacement password";

using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

JsonPtr adopt(json_object* value) {
    return JsonPtr(value, &json_object_put);
}

std::string serialize(json_object* value) {
    return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);
}

bool string_field(json_object* object, const char* name, std::string& value) {
    json_object* field = nullptr;
    if (!json_object_object_get_ex(object, name, &field) ||
        field == nullptr || !json_object_is_type(field, json_type_string)) {
        return false;
    }
    value = json_object_get_string(field);
    return true;
}

class Connection final {
public:
    bool connect(const std::string& host, const std::string& port) {
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
            websocket::stream_base::decorator([](websocket::request_type& request) {
                request.set(http::field::sec_websocket_protocol, subprotocol);
            }));
        websocket::response_type response;
        websocket_.handshake(response, host + ":" + port, std::string(endpoint), error);
        if (error) {
            return false;
        }
        const auto negotiated = response[http::field::sec_websocket_protocol];
        return std::string_view(negotiated.data(), negotiated.size()) == subprotocol;
    }

    bool exchange(const std::string& request, std::string& response) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(request), error);
        if (error) {
            return false;
        }
        beast::flat_buffer buffer;
        websocket_.read(buffer, error);
        if (error || !websocket_.got_text()) {
            return false;
        }
        response = beast::buffers_to_string(buffer.data());
        return true;
    }

    void close() {
        beast::error_code error;
        websocket_.close(websocket::close_code::normal, error);
    }

private:
    asio::io_context io_context_;
    tcp::resolver resolver_{io_context_};
    websocket::stream<tcp::socket> websocket_{io_context_};
};

struct Response final {
    bool ok{false};
    JsonPtr payload{nullptr, &json_object_put};
    std::string error_code;
};

std::optional<Response> request(
    Connection& connection,
    int& next_id,
    const std::string_view operation,
    json_object* payload,
    const std::optional<std::string_view> token = std::nullopt) {
    const std::string id = "step6a-" + std::to_string(next_id++);
    auto message = adopt(json_object_new_object());
    json_object_object_add(message.get(), "type", json_object_new_string("request"));
    json_object_object_add(message.get(), "id", json_object_new_string(id.c_str()));
    json_object_object_add(
        message.get(),
        "service",
        json_object_new_string(std::string(service).c_str()));
    json_object_object_add(
        message.get(),
        "operation",
        json_object_new_string_len(operation.data(), static_cast<int>(operation.size())));
    json_object_object_add(message.get(), "payload", json_object_get(payload));
    if (token.has_value()) {
        auto auth = adopt(json_object_new_object());
        json_object_object_add(auth.get(), "type", json_object_new_string("session"));
        json_object_object_add(
            auth.get(),
            "token",
            json_object_new_string_len(token->data(), static_cast<int>(token->size())));
        json_object_object_add(message.get(), "auth", auth.release());
    }

    std::string response_text;
    if (!connection.exchange(serialize(message.get()), response_text)) {
        return std::nullopt;
    }
    auto parsed = adopt(json_tokener_parse(response_text.c_str()));
    if (!parsed || !json_object_is_type(parsed.get(), json_type_object)) {
        return std::nullopt;
    }

    std::string response_type;
    std::string response_id;
    json_object* ok = nullptr;
    if (!string_field(parsed.get(), "type", response_type) ||
        response_type != "response" ||
        !string_field(parsed.get(), "id", response_id) ||
        response_id != id ||
        !json_object_object_get_ex(parsed.get(), "ok", &ok) ||
        ok == nullptr || !json_object_is_type(ok, json_type_boolean)) {
        return std::nullopt;
    }

    Response response;
    response.ok = json_object_get_boolean(ok) != 0;
    if (response.ok) {
        json_object* response_payload = nullptr;
        if (!json_object_object_get_ex(parsed.get(), "payload", &response_payload) ||
            response_payload == nullptr ||
            !json_object_is_type(response_payload, json_type_object)) {
            return std::nullopt;
        }
        response.payload = adopt(json_object_get(response_payload));
        return response;
    }

    json_object* error = nullptr;
    if (!json_object_object_get_ex(parsed.get(), "error", &error) ||
        error == nullptr || !json_object_is_type(error, json_type_object) ||
        !string_field(error, "code", response.error_code)) {
        return std::nullopt;
    }
    return response;
}

std::optional<std::string> login(
    Connection& connection,
    int& next_id,
    const std::string_view login_name,
    const std::string_view password) {
    auto payload = adopt(json_object_new_object());
    json_object_object_add(
        payload.get(),
        "login",
        json_object_new_string_len(login_name.data(), static_cast<int>(login_name.size())));
    json_object_object_add(
        payload.get(),
        "password",
        json_object_new_string_len(password.data(), static_cast<int>(password.size())));
    const auto response = request(connection, next_id, "login", payload.get());
    if (!response.has_value() || !response->ok) {
        return std::nullopt;
    }
    std::string token;
    if (!string_field(response->payload.get(), "session_token", token)) {
        return std::nullopt;
    }
    return token;
}

bool expect_error(
    const std::optional<Response>& response,
    const std::string_view code,
    const char* label) {
    if (!response.has_value() || response->ok || response->error_code != code) {
        std::cerr << label << " did not return expected error " << code << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: client <host> <port> <admin-login>\n";
        return 2;
    }

    std::string admin_password;
    if (!std::getline(std::cin, admin_password) || admin_password.empty()) {
        std::cerr << "admin password is required on stdin\n";
        return 2;
    }

    Connection connection;
    if (!connection.connect(argv[1], argv[2])) {
        std::cerr << "failed to connect to Service Hub\n";
        return 1;
    }

    int next_id = 1;
    const auto admin_token = login(connection, next_id, argv[3], admin_password);
    if (!admin_token.has_value()) {
        std::cerr << "admin login failed\n";
        return 1;
    }

    {
        auto payload = adopt(json_object_new_object());
        if (!expect_error(
                request(connection, next_id, "list-users", payload.get()),
                "auth.invalid_session",
                "unauthenticated list-users")) {
            return 1;
        }
    }

    {
        auto payload = adopt(json_object_new_object());
        const auto response = request(
            connection,
            next_id,
            "list-users",
            payload.get(),
            *admin_token);
        json_object* users = nullptr;
        if (!response.has_value() || !response->ok ||
            !json_object_object_get_ex(response->payload.get(), "users", &users) ||
            !json_object_is_type(users, json_type_array) ||
            json_object_array_length(users) != 1) {
            std::cerr << "admin list-users failed\n";
            return 1;
        }
    }

    std::string user_id;
    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "login", json_object_new_string("step6a-operator"));
        json_object_object_add(payload.get(), "display_name", json_object_new_string("Step 6A Operator"));
        json_object_object_add(payload.get(), "enabled", json_object_new_boolean(1));
        json_object_object_add(
            payload.get(),
            "password",
            json_object_new_string(std::string(user_password).c_str()));
        const auto response = request(
            connection,
            next_id,
            "create-user",
            payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "admin create-user failed\n";
            return 1;
        }
        json_object* user = nullptr;
        if (!json_object_object_get_ex(response->payload.get(), "user", &user) ||
            !string_field(user, "id", user_id) || user_id.empty()) {
            std::cerr << "create-user response is invalid\n";
            return 1;
        }
    }

    const auto user_token = login(connection, next_id, "step6a-operator", user_password);
    if (!user_token.has_value()) {
        std::cerr << "created user login failed\n";
        return 1;
    }
    {
        auto payload = adopt(json_object_new_object());
        if (!expect_error(
                request(connection, next_id, "list-users", payload.get(), *user_token),
                "access.forbidden",
                "non-admin list-users")) {
            return 1;
        }
    }

    std::string permission_set_id;
    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "name", json_object_new_string("Step 6A editors"));
        json_object* capabilities = json_object_new_array();
        json_object_array_add(capabilities, json_object_new_string("view"));
        json_object_array_add(capabilities, json_object_new_string("edit"));
        json_object_object_add(payload.get(), "capabilities", capabilities);
        const auto response = request(
            connection,
            next_id,
            "create-permission-set",
            payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "create-permission-set failed\n";
            return 1;
        }
        json_object* permission_set = nullptr;
        if (!json_object_object_get_ex(
                response->payload.get(),
                "permission_set",
                &permission_set) ||
            !string_field(permission_set, "id", permission_set_id) ||
            permission_set_id.empty()) {
            std::cerr << "permission-set response is invalid\n";
            return 1;
        }
    }

    {
        auto payload = adopt(json_object_new_object());
        const auto response = request(
            connection,
            next_id,
            "list-permission-sets",
            payload.get(),
            *admin_token);
        json_object* permission_sets = nullptr;
        if (!response.has_value() || !response->ok ||
            !json_object_object_get_ex(
                response->payload.get(),
                "permission_sets",
                &permission_sets) ||
            !json_object_is_type(permission_sets, json_type_array) ||
            json_object_array_length(permission_sets) < 2) {
            std::cerr << "list-permission-sets failed\n";
            return 1;
        }
    }

    auto assignment_payload = adopt(json_object_new_object());
    json_object_object_add(
        assignment_payload.get(),
        "user_id",
        json_object_new_string(user_id.c_str()));
    json_object_object_add(
        assignment_payload.get(),
        "permission_set_id",
        json_object_new_string(permission_set_id.c_str()));
    auto scope = adopt(json_object_new_object());
    json_object_object_add(scope.get(), "kind", json_object_new_string("project"));
    json_object_object_add(scope.get(), "project_id", json_object_new_string("project-step6a"));
    json_object_object_add(assignment_payload.get(), "scope", scope.release());

    {
        const auto response = request(
            connection,
            next_id,
            "assign-access",
            assignment_payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "assign-access failed\n";
            return 1;
        }
    }

    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "user_id", json_object_new_string(user_id.c_str()));
        const auto response = request(
            connection,
            next_id,
            "list-access-assignments",
            payload.get(),
            *admin_token);
        json_object* assignments = nullptr;
        if (!response.has_value() || !response->ok ||
            !json_object_object_get_ex(response->payload.get(), "assignments", &assignments) ||
            !json_object_is_type(assignments, json_type_array) ||
            json_object_array_length(assignments) != 1) {
            std::cerr << "list-access-assignments failed\n";
            return 1;
        }
    }

    {
        const auto response = request(
            connection,
            next_id,
            "remove-access-assignment",
            assignment_payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "remove-access-assignment failed\n";
            return 1;
        }
    }

    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "user_id", json_object_new_string(user_id.c_str()));
        json_object_object_add(
            payload.get(),
            "password",
            json_object_new_string(std::string(replacement_password).c_str()));
        const auto response = request(
            connection,
            next_id,
            "set-user-password",
            payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "set-user-password failed\n";
            return 1;
        }
    }

    if (!login(connection, next_id, "step6a-operator", replacement_password).has_value()) {
        std::cerr << "replacement password login failed\n";
        return 1;
    }

    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "user_id", json_object_new_string(user_id.c_str()));
        json_object_object_add(payload.get(), "enabled", json_object_new_boolean(0));
        const auto response = request(
            connection,
            next_id,
            "set-user-enabled",
            payload.get(),
            *admin_token);
        if (!response.has_value() || !response->ok) {
            std::cerr << "set-user-enabled failed\n";
            return 1;
        }
    }

    {
        auto payload = adopt(json_object_new_object());
        json_object_object_add(payload.get(), "login", json_object_new_string("step6a-operator"));
        json_object_object_add(
            payload.get(),
            "password",
            json_object_new_string(std::string(replacement_password).c_str()));
        if (!expect_error(
                request(connection, next_id, "login", payload.get()),
                "auth.invalid_credentials",
                "disabled user login")) {
            return 1;
        }
    }

    connection.close();
    std::cout << "Users & Access administration integration passed\n";
    return 0;
}
