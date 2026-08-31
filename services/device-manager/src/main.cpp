#include "dispatcher/device_manager/application.hpp"

#include <iostream>

int main(const int argc, char*[]) {
    if (argc != 1) {
        std::cerr << "Usage: dispatcher-device-manager\n";
        return 2;
    }

    const dispatcher::device_manager::Application application;
    return application.run(std::cout);
}
