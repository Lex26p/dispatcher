#include "dispatcher/project_manager/application.hpp"
#include "dispatcher/project_manager/sqlite_project_repository.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    constexpr std::string_view default_database_path = "dispatcher-project-manager.db";

    if (argc > 2) {
        std::cerr << "Usage: dispatcher-project-manager [database-path]\n";
        return 2;
    }

    const std::string_view database_path =
        argc == 2 ? std::string_view(argv[1]) : default_database_path;

    dispatcher::project_manager::SqliteProjectRepository repository{database_path};

    if (!repository.ready()) {
        std::cerr << "Failed to initialize Dispatcher Project Manager storage at "
                  << database_path << ": " << repository.error_message() << '\n';
        return 1;
    }

    std::cout << "Dispatcher Project Manager SQLite storage ready at "
              << database_path << '\n';
    std::cout.flush();

    const dispatcher::project_manager::Application application;
    return application.run(std::cout);
}
