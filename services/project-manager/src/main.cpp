#include "dispatcher/project_manager/application.hpp"

#include <iostream>

int main(int argc, char*[]) {
    if (argc != 1) {
        std::cerr << "Usage: dispatcher-project-manager\n";
        return 2;
    }

    const dispatcher::project_manager::Application application;
    return application.run(std::cout);
}
