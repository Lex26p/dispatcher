#include "dispatcher/data_hub/application.hpp"

#include <iostream>

int main() {
    const dispatcher::data_hub::Application application;
    return application.run(std::cout);
}
