#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <json-c/json.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <poll.h>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <sys/stat.h>

namespace {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using JsonPtr = std::unique_ptr<json_object, decltype(&json_object_put)>;

constexpr std::string_view kSubprotocol = "dispatcher.service-hub.v1";
constexpr std::string_view kProjectService = "project-manager.v1";
constexpr std::string_view kUsersAccessService = "users-access.v1";
constexpr std::string_view kAdminLogin = "step5-admin";
constexpr std::string_view kAdminPassword = "Step5 integration admin password";
constexpr std::string_view kOperatorLogin = "step5-operator";
constexpr std::string_view kOperatorPassword = "Step5 integration operator password";
constexpr std::string_view kExpiringLogin = "step5-expiring";
constexpr std::string_view kExpiringPassword = "Step5 integration expiring password";
constexpr std::string_view kProjectAdminLogin = "step5-project-admin";
constexpr std::string_view kProjectAdminPassword = "Step5 integration project admin password";

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
    if (!json_object_object_get_ex(object, name, &field) ||
        field == nullptr ||
        !json_object_is_type(field, json_type_string)) {
        return false;
    }
    value = json_object_get_string(field);
    return true;
}

class Client final {
public:
    Client() : resolver_(io_context_), websocket_(io_context_) {}

    [[nodiscard]] bool connect(
        const std::string& host,
        const std::string& port) {
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

    [[nodiscard]] bool write(const std::string_view message) {
        beast::error_code error;
        websocket_.text(true);
        websocket_.write(asio::buffer(message), error);
        return !error;
    }

    [[nodiscard]] bool read(
        std::string& message,
        const int timeout_ms = 5000) {
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
    const std::string_view service,
    const std::string_view operation,
    const std::string_view payload_json,
    const std::string_view token = {}) {
    std::string result =
        std::string(R"({"type":"request","id":")") + std::string(id) +
        R"(","service":")" + std::string(service) +
        R"(","operation":")" + std::string(operation) +
        R"(","payload":)" + std::string(payload_json);

    if (!token.empty()) {
        result += R"(,"auth":{"type":"session","token":")" +
                  std::string(token) + R"("})";
    }

    result += R"(,"timeout_ms":5000})";
    return result;
}

[[nodiscard]] bool response_error_code(
    const std::string& text,
    const std::string_view expected_id,
    std::string& code) {
    auto object = parse_json(text);
    if (!object) return false;

    std::string type;
    std::string id;
    json_object* ok = nullptr;
    json_object* error = nullptr;
    return string_field(object.get(), "type", type) &&
           type == "response" &&
           string_field(object.get(), "id", id) &&
           id == expected_id &&
           json_object_object_get_ex(object.get(), "ok", &ok) &&
           ok != nullptr &&
           json_object_is_type(ok, json_type_boolean) &&
           json_object_get_boolean(ok) == 0 &&
           json_object_object_get_ex(object.get(), "error", &error) &&
           error != nullptr &&
           json_object_is_type(error, json_type_object) &&
           string_field(error, "code", code);
}

[[nodiscard]] bool is_error(
    const std::string& text,
    const std::string_view expected_id,
    const std::string_view expected_code) {
    std::string code;
    return response_error_code(text, expected_id, code) &&
           code == expected_code;
}

[[nodiscard]] bool wait_for_project_manager(Client& client) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        const std::string id = "pm-probe-" + std::to_string(attempt);
        if (!client.write(request(
                id,
                kProjectService,
                "list-projects",
                "{}"))) {
            return false;
        }

        std::string response;
        if (!client.read(response)) return false;

        if (is_error(response, id, "auth.invalid_session")) {
            return true;
        }

        if (!is_error(response, id, "hub.unknown_service")) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

[[nodiscard]] bool login(
    Client& client,
    const std::string_view request_id,
    const std::string_view login_name,
    const std::string_view password,
    std::string& token) {
    const std::string payload =
        std::string(R"({"login":")") + std::string(login_name) +
        R"(","password":")" + std::string(password) + R"("})";

    for (int attempt = 0; attempt < 100; ++attempt) {
        const std::string id =
            std::string(request_id) + "-" + std::to_string(attempt);

        if (!client.write(request(
                id,
                kUsersAccessService,
                "login",
                payload))) {
            return false;
        }

        std::string response;
        if (!client.read(response)) return false;

        if (is_error(response, id, "hub.unknown_service")) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto object = parse_json(response);
        if (!object) return false;

        std::string type;
        std::string response_id;
        json_object* ok = nullptr;
        json_object* response_payload = nullptr;
        return string_field(object.get(), "type", type) &&
               type == "response" &&
               string_field(object.get(), "id", response_id) &&
               response_id == id &&
               json_object_object_get_ex(object.get(), "ok", &ok) &&
               ok != nullptr &&
               json_object_get_boolean(ok) != 0 &&
               json_object_object_get_ex(
                   object.get(),
                   "payload",
                   &response_payload) &&
               response_payload != nullptr &&
               string_field(
                   response_payload,
                   "session_token",
                   token) &&
               token.size() == 64;
    }

    return false;
}

[[nodiscard]] bool project_response(
    const std::string& text,
    const std::string_view expected_id,
    std::string& project_id,
    std::string* project_name = nullptr) {
    auto object = parse_json(text);
    if (!object) return false;

    std::string type;
    std::string id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* project = nullptr;
    std::string name;

    if (!string_field(object.get(), "type", type) ||
        type != "response" ||
        !string_field(object.get(), "id", id) ||
        id != expected_id ||
        !json_object_object_get_ex(object.get(), "ok", &ok) ||
        ok == nullptr ||
        json_object_get_boolean(ok) == 0 ||
        !json_object_object_get_ex(object.get(), "payload", &payload) ||
        payload == nullptr ||
        !json_object_object_get_ex(payload, "project", &project) ||
        project == nullptr ||
        !string_field(project, "id", project_id) ||
        !string_field(project, "name", name)) {
        return false;
    }

    if (project_name != nullptr) {
        *project_name = std::move(name);
    }
    return true;
}

[[nodiscard]] bool list_projects(
    const std::string& text,
    const std::string_view expected_id,
    std::vector<std::string>& ids) {
    ids.clear();

    auto object = parse_json(text);
    if (!object) return false;

    std::string type;
    std::string id;
    json_object* ok = nullptr;
    json_object* payload = nullptr;
    json_object* projects = nullptr;

    if (!string_field(object.get(), "type", type) ||
        type != "response" ||
        !string_field(object.get(), "id", id) ||
        id != expected_id ||
        !json_object_object_get_ex(object.get(), "ok", &ok) ||
        ok == nullptr ||
        json_object_get_boolean(ok) == 0 ||
        !json_object_object_get_ex(object.get(), "payload", &payload) ||
        payload == nullptr ||
        !json_object_object_get_ex(payload, "projects", &projects) ||
        projects == nullptr ||
        !json_object_is_type(projects, json_type_array)) {
        return false;
    }

    const auto count = json_object_array_length(projects);
    for (std::size_t index = 0; index < count; ++index) {
        json_object* project = json_object_array_get_idx(projects, index);
        std::string project_id;
        if (project == nullptr ||
            !string_field(project, "id", project_id)) {
            return false;
        }
        ids.push_back(std::move(project_id));
    }

    return true;
}

[[nodiscard]] bool send_and_read(
    Client& client,
    const std::string& message,
    std::string& response) {
    return client.write(message) && client.read(response);
}

[[nodiscard]] bool write_secret_file(
    const std::string_view path,
    const std::string_view value) {
    std::ofstream stream(std::string(path), std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    stream << value << '\n';
    stream.close();
    return stream.good() && ::chmod(std::string(path).c_str(), 0600) == 0;
}

[[nodiscard]] bool read_first_line(
    const std::string_view path,
    std::string& value) {
    std::ifstream stream{std::string(path)};
    return static_cast<bool>(std::getline(stream, value)) && !value.empty();
}

struct ProjectIds final {
    std::string visible;
    std::string hidden;
};

[[nodiscard]] bool write_project_ids(
    const std::string_view path,
    const ProjectIds& ids) {
    std::ofstream stream{std::string(path), std::ios::trunc};
    if (!stream) return false;
    stream << ids.visible << '\n' << ids.hidden << '\n';
    return stream.good();
}

[[nodiscard]] bool read_project_ids(
    const std::string_view path,
    ProjectIds& ids) {
    std::ifstream stream{std::string(path)};
    return static_cast<bool>(
        std::getline(stream, ids.visible) &&
        std::getline(stream, ids.hidden) &&
        !ids.visible.empty() &&
        !ids.hidden.empty());
}

int run_admin_setup(
    Client& client,
    const std::string_view projects_path) {
    std::string response;

    if (!send_and_read(
            client,
            request(
                "unauth-list",
                kProjectService,
                "list-projects",
                "{}"),
            response) ||
        !is_error(response, "unauth-list", "auth.invalid_session")) {
        return fail("unauthenticated Project Manager request must fail closed");
    }

    std::string token;
    if (!login(client, "admin-login", kAdminLogin, kAdminPassword, token)) {
        return fail("bootstrap admin login");
    }

    ProjectIds ids;

    if (!send_and_read(
            client,
            request(
                "create-visible",
                kProjectService,
                "create-project",
                R"({"name":"Visible project","description":"Step 5 visible"})",
                token),
            response) ||
        !project_response(response, "create-visible", ids.visible)) {
        return fail("global admin create visible project");
    }

    if (!send_and_read(
            client,
            request(
                "create-hidden",
                kProjectService,
                "create-project",
                R"({"name":"Hidden project","description":"Step 5 hidden"})",
                token),
            response) ||
        !project_response(response, "create-hidden", ids.hidden)) {
        return fail("global admin create hidden project");
    }

    std::vector<std::string> listed;
    if (!send_and_read(
            client,
            request(
                "admin-list",
                kProjectService,
                "list-projects",
                "{}",
                token),
            response) ||
        !list_projects(response, "admin-list", listed) ||
        listed.size() != 2) {
        return fail("global view should list both projects");
    }

    return write_project_ids(projects_path, ids)
        ? 0
        : fail("write project ids");
}

int run_restricted(
    Client& client,
    const std::string_view projects_path,
    const std::string_view operator_token_path,
    const std::string_view expiring_token_path) {
    ProjectIds ids;
    if (!read_project_ids(projects_path, ids)) {
        return fail("read project ids");
    }

    std::string operator_token;
    if (!login(
            client,
            "operator-login",
            kOperatorLogin,
            kOperatorPassword,
            operator_token)) {
        return fail("operator login");
    }
    if (!write_secret_file(operator_token_path, operator_token)) {
        return fail("write operator token");
    }

    std::string expiring_token;
    if (!login(
            client,
            "expiring-login",
            kExpiringLogin,
            kExpiringPassword,
            expiring_token)) {
        return fail("expiring user login");
    }
    if (!write_secret_file(expiring_token_path, expiring_token)) {
        return fail("write expiring token");
    }

    std::string project_admin_token;
    if (!login(
            client,
            "project-admin-login",
            kProjectAdminLogin,
            kProjectAdminPassword,
            project_admin_token)) {
        return fail("project admin login");
    }

    std::string response;
    std::vector<std::string> listed;
    if (!send_and_read(
            client,
            request(
                "operator-list",
                kProjectService,
                "list-projects",
                "{}",
                operator_token),
            response) ||
        !list_projects(response, "operator-list", listed) ||
        listed.size() != 1 ||
        listed.front() != ids.visible) {
        return fail("list-projects must filter inaccessible projects");
    }

    std::string actual_id;
    if (!send_and_read(
            client,
            request(
                "operator-get-visible",
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.visible + R"("})",
                operator_token),
            response) ||
        !project_response(
            response,
            "operator-get-visible",
            actual_id) ||
        actual_id != ids.visible) {
        return fail("project view access");
    }

    if (!send_and_read(
            client,
            request(
                "operator-get-hidden",
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.hidden + R"("})",
                operator_token),
            response) ||
        !is_error(
            response,
            "operator-get-hidden",
            "access.forbidden")) {
        return fail("inaccessible get-project must be forbidden");
    }

    if (!send_and_read(
            client,
            request(
                "operator-create",
                kProjectService,
                "create-project",
                R"({"name":"Denied create","description":""})",
                operator_token),
            response) ||
        !is_error(response, "operator-create", "access.forbidden")) {
        return fail("create-project requires global admin");
    }

    const std::string update_visible_payload =
        std::string(R"({"id":")") + ids.visible +
        R"(","name":"Visible project updated","description":"edited by project user"})";
    if (!send_and_read(
            client,
            request(
                "operator-update-visible",
                kProjectService,
                "update-project",
                update_visible_payload,
                operator_token),
            response) ||
        !project_response(
            response,
            "operator-update-visible",
            actual_id) ||
        actual_id != ids.visible) {
        return fail("project edit access");
    }

    const std::string update_hidden_payload =
        std::string(R"({"id":")") + ids.hidden +
        R"(","name":"Hidden project changed","description":"must not happen"})";
    if (!send_and_read(
            client,
            request(
                "operator-update-hidden",
                kProjectService,
                "update-project",
                update_hidden_payload,
                operator_token),
            response) ||
        !is_error(
            response,
            "operator-update-hidden",
            "access.forbidden")) {
        return fail("inaccessible update-project must be forbidden");
    }

    if (!send_and_read(
            client,
            request(
                "project-admin-get-hidden",
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.hidden + R"("})",
                project_admin_token),
            response) ||
        !is_error(
            response,
            "project-admin-get-hidden",
            "access.forbidden")) {
        return fail("admin capability must not imply view");
    }

    const std::string admin_update_payload =
        std::string(R"({"id":")") + ids.hidden +
        R"(","name":"Hidden project admin-updated","description":"admin without edit"})";
    if (!send_and_read(
            client,
            request(
                "project-admin-update-hidden",
                kProjectService,
                "update-project",
                admin_update_payload,
                project_admin_token),
            response) ||
        !project_response(
            response,
            "project-admin-update-hidden",
            actual_id) ||
        actual_id != ids.hidden) {
        return fail("project admin capability must authorize update");
    }

    return 0;
}

int run_expected_error(
    Client& client,
    const std::string_view projects_path,
    const std::string_view token_path,
    const std::string_view request_id,
    const std::string_view expected_code) {
    ProjectIds ids;
    std::string token;
    if (!read_project_ids(projects_path, ids) ||
        !read_first_line(token_path, token)) {
        return fail("read integration state");
    }

    std::string response;
    if (!send_and_read(
            client,
            request(
                request_id,
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.visible + R"("})",
                token),
            response) ||
        !is_error(response, request_id, expected_code)) {
        return fail("unexpected authorization error result");
    }

    return 0;
}

int run_recovered(
    Client& client,
    const std::string_view projects_path,
    const std::string_view token_path) {
    ProjectIds ids;
    std::string token;
    if (!read_project_ids(projects_path, ids) ||
        !read_first_line(token_path, token)) {
        return fail("read recovered integration state");
    }

    std::string response;
    std::string actual_id;
    if (!send_and_read(
            client,
            request(
                "recovered-get",
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.visible + R"("})",
                token),
            response) ||
        !project_response(response, "recovered-get", actual_id) ||
        actual_id != ids.visible) {
        return fail("authorization after Users & Access reconnect");
    }

    return 0;
}

int run_revoked(
    Client& client,
    const std::string_view projects_path,
    const std::string_view token_path) {
    ProjectIds ids;
    std::string token;
    if (!read_project_ids(projects_path, ids) ||
        !read_first_line(token_path, token)) {
        return fail("read revoked integration state");
    }

    std::string response;
    std::vector<std::string> listed;
    if (!send_and_read(
            client,
            request(
                "revoked-list",
                kProjectService,
                "list-projects",
                "{}",
                token),
            response) ||
        !list_projects(response, "revoked-list", listed) ||
        !listed.empty()) {
        return fail("revoked project must disappear from list");
    }

    if (!send_and_read(
            client,
            request(
                "revoked-get",
                kProjectService,
                "get-project",
                std::string(R"({"id":")") + ids.visible + R"("})",
                token),
            response) ||
        !is_error(response, "revoked-get", "access.forbidden")) {
        return fail("revoked project get must be forbidden");
    }

    return 0;
}

int run_after_hub_reconnect(
    Client& client,
    const std::string_view projects_path) {
    ProjectIds ids;
    if (!read_project_ids(projects_path, ids)) {
        return fail("read project ids after Hub reconnect");
    }

    std::string token;
    if (!login(
            client,
            "admin-reconnect-login",
            kAdminLogin,
            kAdminPassword,
            token)) {
        return fail("admin login after Hub reconnect");
    }

    std::string response;
    std::vector<std::string> listed;
    if (!send_and_read(
            client,
            request(
                "admin-reconnect-list",
                kProjectService,
                "list-projects",
                "{}",
                token),
            response) ||
        !list_projects(response, "admin-reconnect-list", listed) ||
        listed.size() != 2) {
        return fail("Project Manager authorization after Hub reconnect");
    }

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr
            << "Usage: service-hub-test-client <host> <port> <mode> [paths...]\\n";
        return 2;
    }

    Client client;
    if (!client.connect(argv[1], argv[2])) {
        return fail("connect to Service Hub");
    }
    if (!wait_for_project_manager(client)) {
        return fail("wait for Project Manager provider registration");
    }

    const std::string_view mode = argv[3];

    if (mode == "admin-setup" && argc == 5) {
        return run_admin_setup(client, argv[4]);
    }
    if (mode == "restricted" && argc == 7) {
        return run_restricted(client, argv[4], argv[5], argv[6]);
    }
    if (mode == "unavailable" && argc == 6) {
        return run_expected_error(
            client,
            argv[4],
            argv[5],
            "authorization-unavailable",
            "project.authorization_unavailable");
    }
    if (mode == "recovered" && argc == 6) {
        return run_recovered(client, argv[4], argv[5]);
    }
    if (mode == "revoked" && argc == 6) {
        return run_revoked(client, argv[4], argv[5]);
    }
    if (mode == "disabled" && argc == 6) {
        return run_expected_error(
            client,
            argv[4],
            argv[5],
            "disabled-session",
            "auth.invalid_session");
    }
    if (mode == "expired" && argc == 6) {
        return run_expected_error(
            client,
            argv[4],
            argv[5],
            "expired-session",
            "auth.session_expired");
    }
    if (mode == "after-hub-reconnect" && argc == 5) {
        return run_after_hub_reconnect(client, argv[4]);
    }

    return fail("unknown mode or invalid arguments");
}
