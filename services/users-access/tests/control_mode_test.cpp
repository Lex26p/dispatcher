#include "dispatcher/users_access/control_mode.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/openssl_session_token_codec.hpp"
#include "dispatcher/users_access/sqlite_administration_store.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace ua = dispatcher::users_access;

namespace {

[[noreturn]] void fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

[[nodiscard]] std::filesystem::path make_temp_dir() {
    const auto base = std::filesystem::temp_directory_path();
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto candidate =
            base / ("dispatcher-users-access-control-mode-test-" +
                    std::to_string(std::rand()));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) {
            return candidate;
        }
    }
    fail("failed to create temporary directory");
}

void test_control_mode_lifecycle() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "users-access.db";
    constexpr std::string_view password = "control mode password 123";
    std::int64_t now = 2'000'000;

    ua::SqliteUsersAccessRepository repository{database_path.string()};
    expect(repository.ready(), "SQLite repository should initialize");

    const ua::User user{
        .id = "operator-user",
        .login = "operator",
        .display_name = "Operator",
        .enabled = true,
    };
    expect(
        repository.insert_user(user) == ua::UsersAccessRepositoryStatus::ok,
        "operator user should be inserted");

    ua::OpenSslScryptPasswordHasher password_hasher;
    ua::CredentialVerifier verifier;
    verifier.user_id = user.id;
    expect(
        password_hasher.hash(password, verifier) == ua::PasswordHashStatus::ok,
        "operator password should hash");
    expect(
        repository.set_credential_verifier(verifier) ==
            ua::CredentialRepositoryStatus::ok,
        "operator credential should be stored");

    const ua::PermissionSet viewers{
        .id = "viewers",
        .name = "Viewers",
        .capabilities = {ua::Capability::view},
    };
    const ua::PermissionSet controllers{
        .id = "controllers",
        .name = "Controllers",
        .capabilities = {ua::Capability::control},
    };
    expect(
        repository.insert_permission_set(viewers) ==
            ua::UsersAccessRepositoryStatus::ok,
        "view permission set should be inserted");
    expect(
        repository.insert_permission_set(controllers) ==
            ua::UsersAccessRepositoryStatus::ok,
        "control permission set should be inserted");

    const ua::AccessAssignment view_assignment{
        .user_id = user.id,
        .permission_set_id = viewers.id,
        .scope = ua::AccessScope::project("project-a"),
    };
    const ua::AccessAssignment control_assignment{
        .user_id = user.id,
        .permission_set_id = controllers.id,
        .scope = ua::AccessScope::project("project-a"),
    };
    expect(
        repository.insert_assignment(view_assignment) ==
            ua::UsersAccessRepositoryStatus::ok,
        "view assignment should be inserted");

    ua::UsersAccessManager manager{repository};
    ua::OpenSslSessionTokenCodec token_codec;
    ua::AuthenticationSessionService authentication{
        repository,
        repository,
        repository,
        repository,
        password_hasher,
        token_codec,
        manager,
        [&now] { return now; }};
    expect(authentication.ready(), "authentication service should be ready");

    const auto login = authentication.login(user.login, password);
    expect(login.ok(), "operator should authenticate");
    const std::string token = login.value->token;

    ua::ControlModeService control_mode{
        authentication,
        token_codec,
        manager,
        [&now] { return now; }};

    auto state = control_mode.current(token);
    expect(
        state.ok() && !state.value->enabled &&
            state.value->reason == ua::ControlModeReason::inactive,
        "new session should start with control mode inactive");

    const auto invalid_project = control_mode.enable(token, "   ");
    expect(
        !invalid_project.ok() &&
            invalid_project.error == ua::ControlModeError::invalid_project,
        "control mode should reject whitespace-only project id");

    const auto denied = control_mode.enable(token, "project-a");
    expect(
        !denied.ok() && denied.error == ua::ControlModeError::forbidden,
        "user without control capability must not enable control mode");

    expect(
        repository.insert_assignment(control_assignment) ==
            ua::UsersAccessRepositoryStatus::ok,
        "control assignment should be inserted");

    const auto enabled = control_mode.enable(token, "project-a");
    expect(enabled.ok() && enabled.value->enabled, "control mode should enable");
    expect(
        enabled.value->project_id == "project-a",
        "control mode should bind to the requested project");
    const std::int64_t expected_expiry = now + ua::control_mode_lifetime_ms;
    expect(
        enabled.value->expires_at_unix_ms == expected_expiry,
        "control mode should use the fixed absolute lifetime");

    now += 60'000;
    state = control_mode.current(token);
    expect(
        state.ok() && state.value->enabled &&
            state.value->expires_at_unix_ms == expected_expiry,
        "status reads must not extend control-mode expiration");

    ua::SqliteUsersAccessAdministrationStore administration_store{
        database_path.string()};
    expect(administration_store.ready(), "administration store should reopen schema");
    const ua::SecurityAuditRecord revoke_audit{
        .sequence = 0,
        .occurred_at_unix_ms = now,
        .event = ua::SecurityAuditEventType::access_assignment_removed,
        .actor_user_id = user.id,
        .subject_user_id = user.id,
    };
    expect(
        administration_store.erase_assignment(control_assignment, revoke_audit) ==
            ua::AdministrationStoreStatus::ok,
        "control assignment should be removable");

    state = control_mode.current(token);
    expect(
        state.ok() && !state.value->enabled &&
            state.value->reason == ua::ControlModeReason::access_revoked,
        "active mode should fail closed after control access revocation");

    expect(
        repository.insert_assignment(control_assignment) ==
            ua::UsersAccessRepositoryStatus::ok,
        "control assignment should be restorable");
    const auto reenabled = control_mode.enable(token, "project-a");
    expect(reenabled.ok() && reenabled.value->enabled, "control mode should re-enable");

    now = reenabled.value->expires_at_unix_ms;
    state = control_mode.current(token);
    expect(
        state.ok() && !state.value->enabled &&
            state.value->reason == ua::ControlModeReason::expired,
        "control mode should expire at the absolute deadline");

    const auto enabled_again = control_mode.enable(token, "project-a");
    expect(enabled_again.ok(), "control mode should enable again after expiry");

    ua::ControlModeService restarted_control_mode{
        authentication,
        token_codec,
        manager,
        [&now] { return now; }};
    state = restarted_control_mode.current(token);
    expect(
        state.ok() && !state.value->enabled &&
            state.value->reason == ua::ControlModeReason::inactive,
        "service restart should conservatively reset ephemeral control mode");

    const auto enabled_after_restart =
        restarted_control_mode.enable(token, "project-a");
    expect(enabled_after_restart.ok(), "control mode should enable after restart reset");
    const auto disabled = restarted_control_mode.disable(token);
    expect(
        disabled.ok() && !disabled.value->enabled &&
            disabled.value->reason == ua::ControlModeReason::inactive,
        "explicit disable should return inactive state");

    expect(
        authentication.logout(token) == ua::AuthenticationSessionError::none,
        "logout should invalidate the session");
    restarted_control_mode.forget(token);
    const auto after_logout = restarted_control_mode.current(token);
    expect(
        !after_logout.ok() &&
            after_logout.error == ua::ControlModeError::invalid_session,
        "logged-out session must not retain usable control mode");

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

}  // namespace

int main() {
    test_control_mode_lifecycle();
    std::cout << "Users & Access control mode tests passed\n";
    return 0;
}
