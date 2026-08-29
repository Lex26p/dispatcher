#include "dispatcher/project_manager/application.hpp"
#include "dispatcher/project_manager/project_manager.hpp"
#include "dispatcher/project_manager/service_hub_provider.hpp"
#include "dispatcher/project_manager/sqlite_project_repository.hpp"

#include <iostream>
#include <string_view>
#include <utility>

int main(int argc, char* argv[]) {
    constexpr std::string_view default_database_path = "dispatcher-project-manager.db";
    constexpr std::string_view default_service_hub_address = "127.0.0.1:50052";

    if (argc > 3) {
        std::cerr << "Usage: dispatcher-project-manager [database-path] [service-hub-address]\n";
        return 2;
    }

    const std::string_view database_path =
        argc >= 2 ? std::string_view(argv[1]) : default_database_path;
    const std::string_view service_hub_address =
        argc >= 3 ? std::string_view(argv[2]) : default_service_hub_address;

    auto endpoint = dispatcher::project_manager::parse_service_hub_address(
        service_hub_address);
    if (!endpoint.has_value()) {
        std::cerr << "Invalid Service Hub address: " << service_hub_address << '\n';
        return 2;
    }

    dispatcher::project_manager::SqliteProjectRepository repository{database_path};
    if (!repository.ready()) {
        std::cerr << "Failed to initialize Dispatcher Project Manager storage at "
                  << database_path << ": " << repository.error_message() << '\n';
        return 1;
    }

    std::cout << "Dispatcher Project Manager SQLite storage ready at "
              << database_path << '\n';
    std::cout.flush();

    dispatcher::project_manager::ProjectManager project_manager{repository};
    dispatcher::project_manager::ServiceHubProvider provider{
        project_manager,
        std::move(*endpoint)};

    const dispatcher::project_manager::Application application;
    return application.run(
        std::cout,
        provider,
        database_path,
        service_hub_address);
}
