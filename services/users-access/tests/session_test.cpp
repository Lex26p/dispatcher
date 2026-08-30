#include "dispatcher/users_access/bootstrap.hpp"
#include "dispatcher/users_access/contract.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/openssl_session_token_codec.hpp"
#include "dispatcher/users_access/session.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

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
            base / ("dispatcher-users-access-session-test-" + std::to_string(std::rand()));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) {
            return candidate;
        }
    }
    fail("failed to create temporary directory");
}

[[nodiscard]] bool lowercase_hex_token(const std::string_view token) {
    if (token.size() != ua::session_token_bytes * 2U) {
        return false;
    }
    for (const char character : token) {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f'))) {
            return false;
        }
    }
    return true;
}

void test_contract_constants() {
    expect(
        ua::contract::service_address == "users-access.v1",
        "Users & Access v1 service address should be stable");
    expect(
        ua::contract::operations.size() == 13,
        "Users & Access v1 operation set should stay explicit");
    expect(
        ua::contract::operations.front() == ua::contract::login,
        "login should remain the explicit public entry operation");
}

void test_authentication_session_lifecycle() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "users-access.db";
    constexpr std::string_view password = "correct horse battery staple";

    std::string persisted_token;
    std::string user_id;
    std::int64_t now = 1'000'000;

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "session repository should initialize");

        ua::OpenSslScryptPasswordHasher password_hasher;
        ua::BootstrapService bootstrap{repository, password_hasher};
        const auto bootstrapped = bootstrap.bootstrap_first_admin(
            "admin",
            "Administrator",
            password);
        expect(bootstrapped.ok(), "bootstrap admin should be created");
        user_id = bootstrapped.user->id;

        ua::UsersAccessManager manager{repository};
        ua::OpenSslSessionTokenCodec token_codec;
        ua::AuthenticationSessionService sessions{
            repository,
            repository,
            repository,
            repository,
            password_hasher,
            token_codec,
            manager,
            [&now] { return now; }};
        expect(sessions.ready(), "authentication session service should be ready");

        const auto missing = sessions.login("missing", password);
        expect(!missing.ok(), "missing login should not authenticate");
        expect(
            missing.error == ua::AuthenticationSessionError::invalid_credentials,
            "missing login should use generic invalid credentials error");

        const auto wrong = sessions.login("admin", "wrong password");
        expect(!wrong.ok(), "wrong password should not authenticate");
        expect(
            wrong.error == ua::AuthenticationSessionError::invalid_credentials,
            "wrong password should use generic invalid credentials error");

        const auto login = sessions.login("admin", password);
        expect(login.ok(), "correct credentials should authenticate");
        expect(lowercase_hex_token(login.value->token), "session token should be 256-bit lowercase hex");
        expect(login.value->session.user.id == user_id, "session should bind stable user ID");
        expect(
            login.value->session.absolute_expires_at_unix_ms ==
                now + ua::session_absolute_lifetime_ms,
            "session should have documented absolute lifetime");
        expect(
            login.value->session.idle_timeout_ms == ua::session_idle_timeout_ms,
            "session should expose documented idle timeout");
        persisted_token = login.value->token;

        const auto access = sessions.evaluate_access(
            persisted_token,
            ua::AccessScope::project("project-a"),
            ua::Capability::admin);
        expect(access.ok() && access.value->allowed, "bootstrap admin should have global admin in project scope");

        now += 10LL * 60LL * 1000LL;
        const auto validated = sessions.validate(persisted_token);
        expect(validated.ok(), "active session should validate and refresh activity");
    }

    {
        ua::SqliteUsersAccessRepository repository{database_path.string()};
        expect(repository.ready(), "session storage should reopen");
        ua::OpenSslScryptPasswordHasher password_hasher;
        ua::OpenSslSessionTokenCodec token_codec;
        ua::UsersAccessManager manager{repository};
        ua::AuthenticationSessionService sessions{
            repository,
            repository,
            repository,
            repository,
            password_hasher,
            token_codec,
            manager,
            [&now] { return now; }};

        const auto restored = sessions.validate(persisted_token);
        expect(restored.ok(), "server-side session should survive process/repository reopen");

        now += ua::session_idle_timeout_ms;
        const auto expired = sessions.validate(persisted_token);
        expect(!expired.ok(), "idle session should expire server-side");
        expect(
            expired.error == ua::AuthenticationSessionError::session_expired,
            "idle expiry should report session_expired");

        const auto after_expiry = sessions.validate(persisted_token);
        expect(
            !after_expiry.ok() &&
                after_expiry.error == ua::AuthenticationSessionError::invalid_session,
            "expired session should be removed from storage");

        now += 1;
        const auto second_login = sessions.login("admin", password);
        expect(second_login.ok(), "admin should authenticate again after expiry");
        const std::string disabled_token = second_login.value->token;

        const auto disabled = manager.set_user_enabled(user_id, false);
        expect(disabled.ok() && !disabled.value->enabled, "user should be disabled durably");

        const auto disabled_session = sessions.validate(disabled_token);
        expect(!disabled_session.ok(), "disabled user session should fail closed");
        expect(
            disabled_session.error == ua::AuthenticationSessionError::user_disabled,
            "disabled session should report user_disabled");

        const auto reenabled = manager.set_user_enabled(user_id, true);
        expect(reenabled.ok() && reenabled.value->enabled, "user should be re-enabled for logout test");

        const auto third_login = sessions.login("admin", password);
        expect(third_login.ok(), "re-enabled user should authenticate");
        expect(
            sessions.logout(third_login.value->token) == ua::AuthenticationSessionError::none,
            "logout should invalidate session");
        const auto logged_out = sessions.validate(third_login.value->token);
        expect(
            !logged_out.ok() &&
                logged_out.error == ua::AuthenticationSessionError::invalid_session,
            "logged-out token should no longer validate");

        std::vector<ua::SecurityAuditRecord> audit;
        expect(
            repository.list_security_audit(audit) == ua::SecurityAuditRepositoryStatus::ok,
            "security audit should remain readable");
        bool saw_success = false;
        bool saw_failure = false;
        bool saw_expiry = false;
        bool saw_logout = false;
        bool saw_disabled = false;
        for (const auto& record : audit) {
            saw_success = saw_success || record.event == ua::SecurityAuditEventType::authentication_succeeded;
            saw_failure = saw_failure || record.event == ua::SecurityAuditEventType::authentication_failed;
            saw_expiry = saw_expiry || record.event == ua::SecurityAuditEventType::session_expired;
            saw_logout = saw_logout || record.event == ua::SecurityAuditEventType::session_logged_out;
            saw_disabled = saw_disabled ||
                record.event == ua::SecurityAuditEventType::session_rejected_disabled_user;
        }
        expect(saw_success && saw_failure && saw_expiry && saw_logout && saw_disabled,
               "authentication/session lifecycle should be audited without token material");
    }

    std::ifstream database(database_path, std::ios::binary);
    const std::string bytes{
        std::istreambuf_iterator<char>{database},
        std::istreambuf_iterator<char>{}};
    expect(
        bytes.find(persisted_token) == std::string::npos,
        "raw session token must not be stored in SQLite");

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

}  // namespace

int main() {
    test_contract_constants();
    test_authentication_session_lifecycle();
    std::cout << "Users & Access authentication/session tests passed\n";
    return 0;
}
