#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace dispatcher::project_manager {

enum class AuthorizationScopeKind {
    global,
    project,
};

struct AuthorizationScope final {
    AuthorizationScopeKind kind{AuthorizationScopeKind::global};
    std::string project_id;

    [[nodiscard]] static AuthorizationScope global() {
        return AuthorizationScope{};
    }

    [[nodiscard]] static AuthorizationScope project(std::string id) {
        return AuthorizationScope{
            AuthorizationScopeKind::project,
            std::move(id),
        };
    }
};

enum class AuthorizationResult {
    allowed,
    denied,
    invalid_session,
    session_expired,
    unavailable,
};

class UsersAccessAuthorizationClient final {
public:
    UsersAccessAuthorizationClient(std::string host, std::string port);
    ~UsersAccessAuthorizationClient();

    UsersAccessAuthorizationClient(
        const UsersAccessAuthorizationClient&) = delete;
    UsersAccessAuthorizationClient& operator=(
        const UsersAccessAuthorizationClient&) = delete;

    [[nodiscard]] AuthorizationResult evaluate(
        std::string_view session_token,
        const AuthorizationScope& scope,
        std::string_view capability);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace dispatcher::project_manager
