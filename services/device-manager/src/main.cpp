#include "dispatcher/device_manager/application.hpp"
#include "dispatcher/device_manager/sqlite_metadata_repository.hpp"

#include <iostream>
#include <string_view>

namespace {
constexpr std::string_view default_database_path = "dispatcher-device-manager.db";
}

int main(const int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: dispatcher-device-manager [database-path]\n";
        return 2;
    }

    const std::string_view database_path =
        argc == 2 ? std::string_view(argv[1]) : default_database_path;

    dispatcher::device_manager::SqliteMetadataRepository repository{database_path};
    if (!repository.ready()) {
        std::cerr << "Failed to initialize Dispatcher Device Manager storage at "
                  << database_path << ": " << repository.error_message() << '\n';
        return 1;
    }

    std::cout << "Dispatcher Device Manager SQLite storage ready at "
              << database_path << '\n';
    std::cout.flush();

    const dispatcher::device_manager::Application application;
    return application.run(std::cout);
}
