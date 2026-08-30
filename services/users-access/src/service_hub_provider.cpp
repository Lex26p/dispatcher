#include "dispatcher/users_access/service_hub_provider.hpp"

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
#include <vector>

namespace dispatcher::users_access {
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

[[nodiscard]] bool boolean_field(
    json_object* object,
    const char* name,
    bool& value) {
    json_object* field = nullptr;
    if (!json_object_object_get_ex(object, name, &field) ||
        field == nullptr ||
        !json_object_is_type(field, json_type_boolean)) {
        return false;
    }
    value = json_object_get_boolean(field) != 0;
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

[[nodiscard]] std::string success_response(
    const std::string_view request_id,
    json_object* payload) {
    auto response = adopt_json(json_object_new_object());
    json_object_object_add(response.get(), "type", json_object_new_string("response"));
    json_object_object_add(
        response.get(),
        "id",
        json_object_new_string_len(
            request_id.data(),
            static_cast<int>(request_id.size())));
    json_object_object_add(response.get(), "ok", json_object_new_boolean(1));
    json_object_object_add(response.get(), "payload", payload);
    return serialize(response.get());
}

[[nodiscard]] std::string empty_success_response(
    const std::string_view request_id) {
    return success_response(request_id, json_object_new_object());
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
        json_object_new_string_len(
            request_id.data(),
            static_cast<int>(request_id.size())));
    json_object_object_add(response.get(), "ok", json_object_new_boolean(0));
    json_object_object_add(response.get(), "error", error.release());
    return serialize(response.get());
}

[[nodiscard]] json_object* user_json(const User& user) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, "id", json_object_new_string(user.id.c_str()));
    json_object_object_add(object, "login", json_object_new_string(user.login.c_str()));
    json_object_object_add(
        object,
        "display_name",
        json_object_new_string(user.display_name.c_str()));
    json_object_object_add(object, "enabled", json_object_new_boolean(user.enabled));
    return object;
}

[[nodiscard]] json_object* scope_json(const AccessScope& scope) {
    json_object* object = json_object_new_object();
    if (scope.kind == AccessScopeKind::global) {
        json_object_object_add(object, "kind", json_object_new_string("global"));
    } else {
        json_object_object_add(object, "kind", json_object_new_string("project"));
        json_object_object_add(
            object,
            "project_id",
            json_object_new_string(scope.project_id.c_str()));
    }
    return object;
}

[[nodiscard]] json_object* permission_set_json(
    const PermissionSet& permission_set) {
    json_object* object = json_object_new_object();
    json_object_object_add(
        object,
        "id",
        json_object_new_string(permission_set.id.c_str()));
    json_object_object_add(
        object,
        "name",
        json_object_new_string(permission_set.name.c_str()));

    json_object* capabilities = json_object_new_array();
    for (const auto capability : permission_set.capabilities) {
        const auto name = capability_name(capability);
        json_object_array_add(
            capabilities,
            json_object_new_string_len(name.data(), static_cast<int>(name.size())));
    }
    json_object_object_add(object, "capabilities", capabilities);
    return object;
}

[[nodiscard]] json_object* assignment_json(
    const AccessAssignment& assignment) {
    json_object* object = json_object_new_object();
    json_object_object_add(
        object,
        "user_id",
        json_object_new_string(assignment.user_id.c_str()));
    json_object_object_add(
        object,
        "permission_set_id",
        json_object_new_string(assignment.permission_set_id.c_str()));
    json_object_object_add(object, "scope", scope_json(assignment.scope));
    return object;
}

[[nodiscard]] json_object* session_json(const AuthenticatedSession& session) {
    json_object* object = json_object_new_object();
    json_object_object_add(object, "user", user_json(session.user));
    json_object_object_add(
        object,
        "issued_at_unix_ms",
        json_object_new_int64(session.issued_at_unix_ms));
    json_object_object_add(
        object,
        "absolute_expires_at_unix_ms",
        json_object_new_int64(session.absolute_expires_at_unix_ms));
    json_object_object_add(
        object,
        "idle_timeout_ms",
        json_object_new_int64(session.idle_timeout_ms));
    return object;
}

[[nodiscard]] std::pair<std::string_view, std::string_view> session_error_info(
    const AuthenticationSessionError error) {
    switch (error) {
    case AuthenticationSessionError::invalid_credentials:
        return {"auth.invalid_credentials", "Authentication credentials were rejected"};
    case AuthenticationSessionError::invalid_session:
    case AuthenticationSessionError::user_disabled:
        return {"auth.invalid_session", "Authenticated session is invalid"};
    case AuthenticationSessionError::session_expired:
        return {"auth.session_expired", "Authenticated session has expired"};
    case AuthenticationSessionError::storage_error:
        return {"access.storage_error", "Users & Access storage operation failed"};
    case AuthenticationSessionError::crypto_error:
    case AuthenticationSessionError::session_generation_failed:
        return {"auth.crypto_error", "Authentication cryptographic operation failed"};
    case AuthenticationSessionError::none:
        break;
    }
    return {"access.internal_error", "Users & Access operation failed"};
}

[[nodiscard]] std::string session_error_response(
    const std::string_view request_id,
    const AuthenticationSessionError error) {
    const auto [code, message] = session_error_info(error);
    return error_response(request_id, code, message);
}

[[nodiscard]] std::pair<std::string_view, std::string_view> administration_error_info(
    const UsersAccessAdministrationError error) {
    switch (error) {
    case UsersAccessAdministrationError::invalid_login:
        return {"access.invalid_request", "User login must contain a non-whitespace character"};
    case UsersAccessAdministrationError::login_too_long:
        return {"access.invalid_request", "User login exceeds the supported UTF-8 payload size"};
    case UsersAccessAdministrationError::display_name_too_long:
        return {"access.invalid_request", "User display name exceeds the supported UTF-8 payload size"};
    case UsersAccessAdministrationError::invalid_password:
        return {"access.invalid_request", "Password is outside the supported Users & Access limits"};
    case UsersAccessAdministrationError::password_too_short:
        return {"access.invalid_request", "Password must contain at least 15 bytes"};
    case UsersAccessAdministrationError::login_conflict:
    case UsersAccessAdministrationError::assignment_conflict:
    case UsersAccessAdministrationError::assignment_not_found:
        return {"access.conflict", "Users & Access administration conflict"};
    case UsersAccessAdministrationError::invalid_permission_set_name:
        return {"access.invalid_request", "Permission set name must contain a non-whitespace character"};
    case UsersAccessAdministrationError::permission_set_name_too_long:
        return {"access.invalid_request", "Permission set name exceeds the supported UTF-8 payload size"};
    case UsersAccessAdministrationError::invalid_scope:
        return {"access.invalid_request", "Access assignment scope is invalid"};
    case UsersAccessAdministrationError::invalid_capability:
        return {"access.invalid_request", "Capability name is invalid"};
    case UsersAccessAdministrationError::user_not_found:
        return {"access.user_not_found", "Target user does not exist"};
    case UsersAccessAdministrationError::permission_set_not_found:
        return {"access.permission_set_not_found", "Target permission set does not exist"};
    case UsersAccessAdministrationError::storage_error:
        return {"access.storage_error", "Users & Access storage operation failed"};
    case UsersAccessAdministrationError::crypto_error:
        return {"auth.crypto_error", "Credential hashing failed"};
    case UsersAccessAdministrationError::id_generation_failed:
        return {"access.internal_error", "Stable identifier generation failed"};
    case UsersAccessAdministrationError::none:
        break;
    }
    return {"access.internal_error", "Users & Access administration failed"};
}

[[nodiscard]] std::string administration_error_response(
    const std::string_view request_id,
    const UsersAccessAdministrationError error) {
    const auto [code, message] = administration_error_info(error);
    return error_response(request_id, code, message);
}

[[nodiscard]] bool session_auth_token(
    json_object* message,
    std::string& token) {
    json_object* auth = nullptr;
    if (!json_object_object_get_ex(message, "auth", &auth) ||
        !object_has_only_fields(auth, {"type", "token"})) {
        return false;
    }

    std::string type;
    return string_field(auth, "type", type) &&
           type == "session" &&
           string_field(auth, "token", token);
}

[[nodiscard]] std::optional<Capability> parse_capability(
    const std::string_view name) {
    for (const auto capability : all_capabilities) {
        if (capability_name(capability) == name) {
            return capability;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool parse_capabilities(
    json_object* value,
    std::vector<Capability>& capabilities) {
    if (value == nullptr || !json_object_is_type(value, json_type_array)) {
        return false;
    }

    capabilities.clear();
    const auto count = json_object_array_length(value);
    for (std::size_t index = 0; index < count; ++index) {
        json_object* item = json_object_array_get_idx(value, index);
        if (item == nullptr || !json_object_is_type(item, json_type_string)) {
            return false;
        }
        const auto parsed = parse_capability(json_object_get_string(item));
        if (!parsed.has_value()) {
            return false;
        }
        capabilities.push_back(*parsed);
    }
    return true;
}

[[nodiscard]] std::optional<AccessScope> parse_scope(json_object* value) {
    if (!object_has_only_fields(value, {"kind", "project_id"})) {
        return std::nullopt;
    }

    std::string kind;
    if (!string_field(value, "kind", kind)) {
        return std::nullopt;
    }

    json_object* project_id_value = nullptr;
    const bool has_project_id =
        json_object_object_get_ex(value, "project_id", &project_id_value);

    if (kind == "global") {
        if (has_project_id) {
            return std::nullopt;
        }
        return AccessScope::global();
    }
    if (kind != "project" || !has_project_id ||
        !json_object_is_type(project_id_value, json_type_string)) {
        return std::nullopt;
    }

    const std::string project_id = json_object_get_string(project_id_value);
    if (project_id.empty()) {
        return std::nullopt;
    }
    return AccessScope::project(project_id);
}

[[nodiscard]] json_object* access_evaluation_json(
    const AccessEvaluation& evaluation) {
    json_object* object = json_object_new_object();
    json_object_object_add(
        object,
        "allowed",
        json_object_new_boolean(evaluation.allowed));

    json_object* capabilities = json_object_new_array();
    for (const auto capability : evaluation.effective_capabilities) {
        const auto name = capability_name(capability);
        json_object_array_add(
            capabilities,
            json_object_new_string_len(name.data(), static_cast<int>(name.size())));
    }
    json_object_object_add(object, "effective_capabilities", capabilities);
    return object;
}

[[nodiscard]] bool is_administration_operation(const std::string_view operation) {
    return operation == contract::list_users ||
           operation == contract::create_user ||
           operation == contract::set_user_enabled ||
           operation == contract::set_user_password ||
           operation == contract::list_permission_sets ||
           operation == contract::create_permission_set ||
           operation == contract::list_access_assignments ||
           operation == contract::assign_access ||
           operation == contract::remove_access_assignment;
}

[[nodiscard]] std::optional<std::string> require_global_admin(
    AuthenticationSessionService& authentication,
    const std::string_view request_id,
    const std::string_view token,
    std::string& actor_user_id) {
    auto session = authentication.validate(token);
    if (!session.ok()) {
        return session_error_response(request_id, session.error);
    }

    auto result = authentication.evaluate_access(
        token,
        AccessScope::global(),
        Capability::admin);
    if (!result.ok()) {
        return session_error_response(request_id, result.error);
    }
    if (!result.value->allowed) {
        return error_response(
            request_id,
            "access.forbidden",
            "Global admin capability is required");
    }

    actor_user_id = session.value->user.id;
    return std::nullopt;
}

[[nodiscard]] std::string handle_administration_request(
    UsersAccessAdministrationService& administration,
    const std::string_view actor_user_id,
    const std::string_view request_id,
    const std::string_view operation,
    json_object* payload) {
    if (operation == contract::list_users) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(request_id, "access.invalid_request", "list-users payload must be empty");
        }
        auto result = administration.list_users();
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object* users = json_object_new_array();
        for (const auto& user : *result.value) {
            json_object_array_add(users, user_json(user));
        }
        json_object_object_add(response.get(), "users", users);
        return success_response(request_id, response.release());
    }

    if (operation == contract::create_user) {
        if (!object_has_only_fields(payload, {"login", "display_name", "enabled", "password"})) {
            return error_response(request_id, "access.invalid_request", "create-user payload shape is invalid");
        }
        std::string login;
        std::string display_name;
        std::string password;
        bool enabled = true;
        if (!string_field(payload, "login", login) ||
            !string_field(payload, "display_name", display_name) ||
            !boolean_field(payload, "enabled", enabled) ||
            !string_field(payload, "password", password)) {
            return error_response(request_id, "access.invalid_request", "create-user requires login, display_name, enabled and password");
        }
        auto result = administration.create_user(actor_user_id, CreateAdministrationUserInput{
            .login = std::move(login),
            .display_name = std::move(display_name),
            .enabled = enabled,
            .password = std::move(password),
        });
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object_object_add(response.get(), "user", user_json(*result.value));
        return success_response(request_id, response.release());
    }

    if (operation == contract::set_user_enabled) {
        if (!object_has_only_fields(payload, {"user_id", "enabled"})) {
            return error_response(request_id, "access.invalid_request", "set-user-enabled payload shape is invalid");
        }
        std::string user_id;
        bool enabled = true;
        if (!string_field(payload, "user_id", user_id) || user_id.empty() ||
            !boolean_field(payload, "enabled", enabled)) {
            return error_response(request_id, "access.invalid_request", "set-user-enabled requires user_id and enabled");
        }
        auto result = administration.set_user_enabled(actor_user_id, user_id, enabled);
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object_object_add(response.get(), "user", user_json(*result.value));
        return success_response(request_id, response.release());
    }

    if (operation == contract::set_user_password) {
        if (!object_has_only_fields(payload, {"user_id", "password"})) {
            return error_response(request_id, "access.invalid_request", "set-user-password payload shape is invalid");
        }
        std::string user_id;
        std::string password;
        if (!string_field(payload, "user_id", user_id) || user_id.empty() ||
            !string_field(payload, "password", password)) {
            return error_response(request_id, "access.invalid_request", "set-user-password requires user_id and password");
        }
        const auto error = administration.set_user_password(actor_user_id, user_id, password);
        return error == UsersAccessAdministrationError::none
            ? empty_success_response(request_id)
            : administration_error_response(request_id, error);
    }

    if (operation == contract::list_permission_sets) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(request_id, "access.invalid_request", "list-permission-sets payload must be empty");
        }
        auto result = administration.list_permission_sets();
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object* values = json_object_new_array();
        for (const auto& permission_set : *result.value) {
            json_object_array_add(values, permission_set_json(permission_set));
        }
        json_object_object_add(response.get(), "permission_sets", values);
        return success_response(request_id, response.release());
    }

    if (operation == contract::create_permission_set) {
        if (!object_has_only_fields(payload, {"name", "capabilities"})) {
            return error_response(request_id, "access.invalid_request", "create-permission-set payload shape is invalid");
        }
        std::string name;
        json_object* capabilities_value = nullptr;
        std::vector<Capability> capabilities;
        if (!string_field(payload, "name", name) ||
            !json_object_object_get_ex(payload, "capabilities", &capabilities_value) ||
            !parse_capabilities(capabilities_value, capabilities)) {
            return error_response(request_id, "access.invalid_request", "create-permission-set requires name and valid capabilities");
        }
        auto result = administration.create_permission_set(actor_user_id, CreatePermissionSetInput{
            .name = std::move(name),
            .capabilities = std::move(capabilities),
        });
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object_object_add(
            response.get(),
            "permission_set",
            permission_set_json(*result.value));
        return success_response(request_id, response.release());
    }

    if (operation == contract::list_access_assignments) {
        if (!object_has_only_fields(payload, {"user_id"})) {
            return error_response(request_id, "access.invalid_request", "list-access-assignments payload shape is invalid");
        }
        std::string user_id;
        std::optional<std::string_view> filter;
        json_object* user_id_value = nullptr;
        if (json_object_object_get_ex(payload, "user_id", &user_id_value)) {
            if (user_id_value == nullptr ||
                !json_object_is_type(user_id_value, json_type_string)) {
                return error_response(request_id, "access.invalid_request", "user_id must be a string when present");
            }
            user_id = json_object_get_string(user_id_value);
            if (user_id.empty()) {
                return error_response(request_id, "access.invalid_request", "user_id must not be empty");
            }
            filter = user_id;
        }
        auto result = administration.list_assignments(filter);
        if (!result.ok()) {
            return administration_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object* values = json_object_new_array();
        for (const auto& assignment : *result.value) {
            json_object_array_add(values, assignment_json(assignment));
        }
        json_object_object_add(response.get(), "assignments", values);
        return success_response(request_id, response.release());
    }

    if (operation == contract::assign_access ||
        operation == contract::remove_access_assignment) {
        if (!object_has_only_fields(payload, {"user_id", "permission_set_id", "scope"})) {
            return error_response(request_id, "access.invalid_request", "access assignment payload shape is invalid");
        }
        std::string user_id;
        std::string permission_set_id;
        json_object* scope_value = nullptr;
        if (!string_field(payload, "user_id", user_id) || user_id.empty() ||
            !string_field(payload, "permission_set_id", permission_set_id) ||
            permission_set_id.empty() ||
            !json_object_object_get_ex(payload, "scope", &scope_value)) {
            return error_response(request_id, "access.invalid_request", "access assignment requires user_id, permission_set_id and scope");
        }
        const auto scope = parse_scope(scope_value);
        if (!scope.has_value()) {
            return error_response(request_id, "access.invalid_request", "access assignment scope is invalid");
        }

        if (operation == contract::assign_access) {
            auto result = administration.assign(actor_user_id, CreateAccessAssignmentInput{
                .user_id = std::move(user_id),
                .permission_set_id = std::move(permission_set_id),
                .scope = *scope,
            });
            if (!result.ok()) {
                return administration_error_response(request_id, result.error);
            }
            auto response = adopt_json(json_object_new_object());
            json_object_object_add(
                response.get(),
                "assignment",
                assignment_json(*result.value));
            return success_response(request_id, response.release());
        }

        const auto error = administration.remove_assignment(actor_user_id, AccessAssignment{
            .user_id = std::move(user_id),
            .permission_set_id = std::move(permission_set_id),
            .scope = *scope,
        });
        return error == UsersAccessAdministrationError::none
            ? empty_success_response(request_id)
            : administration_error_response(request_id, error);
    }

    return error_response(
        request_id,
        "access.unknown_operation",
        "Users & Access does not support the requested administration operation");
}

[[nodiscard]] std::string handle_request(
    AuthenticationSessionService& authentication,
    UsersAccessAdministrationService& administration,
    const std::string_view request_id,
    const std::string_view operation,
    json_object* message,
    json_object* payload) {
    if (operation == contract::login) {
        if (!object_has_only_fields(payload, {"login", "password"})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "login payload must contain only login and password");
        }

        std::string login;
        std::string password;
        if (!string_field(payload, "login", login) ||
            !string_field(payload, "password", password)) {
            return error_response(
                request_id,
                "access.invalid_request",
                "login requires string login and password");
        }

        auto result = authentication.login(login, password);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }

        auto response = adopt_json(json_object_new_object());
        json_object_object_add(
            response.get(),
            "session_token",
            json_object_new_string(result.value->token.c_str()));
        json_object_object_add(
            response.get(),
            "session",
            session_json(result.value->session));
        return success_response(request_id, response.release());
    }

    std::string token;
    if (!session_auth_token(message, token)) {
        return error_response(
            request_id,
            "auth.invalid_session",
            "Protected Users & Access operation requires session authentication");
    }

    if (operation == contract::logout) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "logout payload must be an empty object");
        }
        const auto error = authentication.logout(token);
        return error == AuthenticationSessionError::none
            ? empty_success_response(request_id)
            : session_error_response(request_id, error);
    }

    if (operation == contract::current_session) {
        if (!object_has_only_fields(payload, {})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "current-session payload must be an empty object");
        }
        auto result = authentication.validate(token);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }
        auto response = adopt_json(json_object_new_object());
        json_object_object_add(
            response.get(),
            "session",
            session_json(*result.value));
        return success_response(request_id, response.release());
    }

    if (operation == contract::evaluate_access) {
        if (!object_has_only_fields(payload, {"scope", "capability"})) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access requires scope and capability");
        }

        json_object* scope_value = nullptr;
        std::string capability_name_value;
        if (!json_object_object_get_ex(payload, "scope", &scope_value) ||
            !string_field(payload, "capability", capability_name_value)) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access requires scope and capability");
        }

        const auto scope = parse_scope(scope_value);
        const auto capability = parse_capability(capability_name_value);
        if (!scope.has_value() || !capability.has_value()) {
            return error_response(
                request_id,
                "access.invalid_request",
                "evaluate-access scope or capability is invalid");
        }

        auto result = authentication.evaluate_access(token, *scope, *capability);
        if (!result.ok()) {
            return session_error_response(request_id, result.error);
        }
        return success_response(
            request_id,
            access_evaluation_json(*result.value));
    }

    if (is_administration_operation(operation)) {
        std::string actor_user_id;
        if (const auto denied = require_global_admin(
                authentication,
                request_id,
                token,
                actor_user_id);
            denied.has_value()) {
            return *denied;
        }
        return handle_administration_request(
            administration,
            actor_user_id,
            request_id,
            operation,
            payload);
    }

    return error_response(
        request_id,
        "access.unknown_operation",
        "Users & Access does not support the requested operation");
}

class HubConnection final {
public:
    HubConnection()
        : resolver_(io_context_), websocket_(io_context_) {}

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
    auto registration = adopt_json(json_object_new_object());
    json_object_object_add(registration.get(), "type", json_object_new_string("register"));
    json_object_object_add(
        registration.get(),
        "service",
        json_object_new_string(
            std::string(ServiceHubProvider::service_address).c_str()));
    if (!connection.write(serialize(registration.get()))) {
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
    return string_field(response.get(), "type", type) &&
           type == "registered" &&
           string_field(response.get(), "service", service) &&
           service == ServiceHubProvider::service_address;
}

[[nodiscard]] bool handle_hub_message(
    HubConnection& connection,
    AuthenticationSessionService& authentication,
    UsersAccessAdministrationService& administration,
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

    return connection.write(handle_request(
        authentication,
        administration,
        request_id,
        operation,
        message.get(),
        payload));
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
        port_value = port_value * 10U +
                     static_cast<unsigned int>(character - '0');
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
    AuthenticationSessionService& authentication,
    UsersAccessAdministrationService& administration,
    ServiceHubEndpoint endpoint)
    : authentication_(authentication),
      administration_(administration),
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
                healthy = handle_hub_message(
                    connection,
                    authentication_,
                    administration_,
                    message);
                break;
            }
        }

        connection.close();
        if (!stop_requested_.load()) {
            interruptible_sleep(stop_requested_);
        }
    }
}

}  // namespace dispatcher::users_access
