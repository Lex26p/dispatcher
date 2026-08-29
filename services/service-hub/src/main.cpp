#include "dispatcher/service_hub/application.hpp"

#include <iostream>

int main(int argc, char*[]) {
    if (argc != 1) {
        std::cerr << "Usage: dispatcher-service-hub\n";
        return 2;
    }

    const dispatcher::service_hub::Application application;
    return application.run(std::cout);
}
