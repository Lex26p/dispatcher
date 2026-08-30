#include "dispatcher/users_access/administration.hpp"
#include "dispatcher/users_access/application.hpp"
#include "dispatcher/users_access/bootstrap.hpp"
#include "dispatcher/users_access/openssl_scrypt_password_hasher.hpp"
#include "dispatcher/users_access/openssl_session_token_codec.hpp"
#include "dispatcher/users_access/service_hub_provider.hpp"
#include "dispatcher/users_access/session.hpp"
#include "dispatcher/users_access/sqlite_administration_store.hpp"
#include "dispatcher/users_access/sqlite_users_access_repository.hpp"
#include "dispatcher/users_access/users_access_manager.hpp"

#include <openssl/crypto.h>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <termios.h>
#include <unistd.h>

namespace {

constexpr std::string_view default_database_path = "dispatcher-users-access.db";
constexpr std::string_view default_service_hub_address = "127.0.0.1:50052";

void print_usage() {
    std::cerr
        << "Usage:\n"
        << "  dispatcher-users-access [database-path] [service-hub-address]\n"
        << "  dispatcher-users-access --bootstrap-admin <login> <display-name> [database-path]\n";
}

[[nodiscard]] std::optional<std::string> read_secret_line(
    const std::string_view prompt) {
    const bool terminal = isatty(STDIN_FILENO) != 0;
    termios previous{};
    bool echo_disabled = false;

    if (terminal && tcgetattr(STDIN_FILENO, &previous) == 0) {
        termios hidden = previous;
        hidden.c_lflag &= static_cast<tcflag_t>(~ECHO);
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) == 0) {
            echo_disabled = true;
        }
    }

    if (terminal) {
        std::cerr << prompt;
        std::cerr.flush();
    }

    std::string value;
    const bool read = static_cast<bool>(std::getline(std::cin, value));

    if (echo_disabled) {
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous);
        std::cerr << '\n';
    }

    if (!read) {
        if (echo_disabled) {
            OPENSSL_cleanse(value.data(), value.size());
        }
        return std::nullopt;
    }
    return value;
}

void cleanse(std::string& value) noexcept {
    if (!value.empty()) {
        OPENSSL_cleanse(value.data(), value.size());
    }
    value.clear();
}

[[nodiscard]] const char* bootstrap_error_message(
    const dispatcher::users_access::BootstrapError error) noexcept {
    using dispatcher::users_access::BootstrapError;
    switch (error) {
    case BootstrapError::none:
        return "none";
    case BootstrapError::invalid_login:
        return "login must contain a non-whitespace character";
    case BootstrapError::login_too_long:
        return "login is too long";
    case BootstrapError::display_name_too_long:
        return "display name is too long";
    case BootstrapError::password_too_short:
        return "password is too short";
    case BootstrapError::password_too_long:
        return "password is too long";
    case BootstrapError::already_initialized:
        return "users storage is already initialized";
    case BootstrapError::crypto_error:
        return "password verifier generation failed";
    case BootstrapError::storage_error:
        return "bootstrap storage transaction failed";
    case BootstrapError::id_generation_failed:
        return "stable identifier generation failed";
    }
    return "unknown bootstrap error";
}

}  // namespace

int main(int argc, char* argv[]) {
    bool bootstrap = false;
    std::string_view login;
    std::string_view display_name;
    std::string_view database_path = default_database_path;
    std::string_view service_hub_address = default_service_hub_address;

    if (argc == 1) {
        // Normal service startup with default storage and Hub paths.
    } else if (argc == 2 && std::string_view(argv[1]) != "--bootstrap-admin") {
        database_path = argv[1];
    } else if (argc == 3 && std::string_view(argv[1]) != "--bootstrap-admin") {
        database_path = argv[1];
        service_hub_address = argv[2];
    } else if (
        (argc == 4 || argc == 5) &&
        std::string_view(argv[1]) == "--bootstrap-admin") {
        bootstrap = true;
        login = argv[2];
        display_name = argv[3];
        if (argc == 5) {
            database_path = argv[4];
        }
    } else {
        print_usage();
        return 2;
    }

    dispatcher::users_access::SqliteUsersAccessRepository repository{database_path};
    if (!repository.ready()) {
        std::cerr << "Failed to initialize Dispatcher Users & Access storage at "
                  << database_path << ": " << repository.error_message() << '\n';
        return 1;
    }

    if (bootstrap) {
        auto password = read_secret_line("Bootstrap password: ");
        if (!password.has_value()) {
            std::cerr << "Failed to read bootstrap password\n";
            return 1;
        }

        auto confirmation = read_secret_line("Confirm bootstrap password: ");
        if (!confirmation.has_value()) {
            cleanse(*password);
            std::cerr << "Failed to read bootstrap password confirmation\n";
            return 1;
        }

        if (*password != *confirmation) {
            cleanse(*password);
            cleanse(*confirmation);
            std::cerr << "Bootstrap password confirmation does not match\n";
            return 1;
        }

        dispatcher::users_access::OpenSslScryptPasswordHasher password_hasher;
        dispatcher::users_access::BootstrapService bootstrap_service{
            repository,
            password_hasher};

        const auto result = bootstrap_service.bootstrap_first_admin(
            login,
            display_name,
            *password);

        cleanse(*password);
        cleanse(*confirmation);

        if (!result.ok()) {
            std::cerr << "Failed to bootstrap first administrator: "
                      << bootstrap_error_message(result.error) << '\n';
            return 1;
        }

        std::cout << "Dispatcher Users & Access first administrator created"
                  << " (user id " << result.user->id
                  << ", login " << result.user->login << ")\n";
        return 0;
    }

    const auto endpoint =
        dispatcher::users_access::parse_service_hub_address(service_hub_address);
    if (!endpoint.has_value()) {
        std::cerr << "Invalid Service Hub address: " << service_hub_address << '\n';
        return 2;
    }

    dispatcher::users_access::SqliteUsersAccessAdministrationStore administration_store{
        database_path};
    if (!administration_store.ready()) {
        std::cerr << "Failed to initialize Dispatcher Users & Access administration storage at "
                  << database_path << ": " << administration_store.error_message() << '\n';
        return 1;
    }

    dispatcher::users_access::OpenSslScryptPasswordHasher password_hasher;
    dispatcher::users_access::OpenSslSessionTokenCodec token_codec;
    dispatcher::users_access::UsersAccessManager access_manager{repository};
    dispatcher::users_access::AuthenticationSessionService authentication{
        repository,
        repository,
        repository,
        repository,
        password_hasher,
        token_codec,
        access_manager};
    if (!authentication.ready()) {
        std::cerr << "Failed to initialize Dispatcher Users & Access authentication service\n";
        return 1;
    }

    dispatcher::users_access::UsersAccessAdministrationService administration{
        repository,
        repository,
        administration_store,
        password_hasher,
        access_manager};

    dispatcher::users_access::ServiceHubProvider provider{
        authentication,
        administration,
        *endpoint};

    std::cout << "Dispatcher Users & Access SQLite storage ready at "
              << database_path << '\n';
    std::cout.flush();

    const dispatcher::users_access::Application application;
    return application.run(
        std::cout,
        provider,
        database_path,
        service_hub_address);
}
