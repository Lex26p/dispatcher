#include "dispatcher/data_hub/application.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    constexpr std::string_view default_address = "0.0.0.0:50051";

    if (argc > 2) {
        std::cerr << "Usage: dispatcher-data-hub [listen-address]\n";
        return 2;
    }

    const std::string_view listen_address =
        argc == 2 ? std::string_view(argv[1]) : default_address;

    const dispatcher::data_hub::Application application;
    return application.run(std::cout, listen_address);
}
