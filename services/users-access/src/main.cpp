#include "dispatcher/users_access/application.hpp"

#include <iostream>

int main(int argc, char*[]) {
    if (argc != 1) {
        std::cerr << "Usage: dispatcher-users-access\n";
        return 2;
    }

    const dispatcher::users_access::Application application;
    return application.run(std::cout);
}
