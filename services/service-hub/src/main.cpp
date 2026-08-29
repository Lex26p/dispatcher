#include "dispatcher/service_hub/application.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    constexpr std::string_view default_address = "0.0.0.0:50052";

    if (argc > 2) {
        std::cerr << "Usage: dispatcher-service-hub [listen-address]\n";
        return 2;
    }

    const std::string_view listen_address =
        argc == 2 ? std::string_view(argv[1]) : default_address;

    const dispatcher::service_hub::Application application;
    return application.run(std::cout, listen_address);
}
