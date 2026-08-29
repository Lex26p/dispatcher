#include "dispatcher/service_hub/application.hpp"

#include <iostream>
#include <sstream>
#include <string_view>

namespace {

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

int test_service_name() {
    using dispatcher::service_hub::Application;

    if (Application::service_name() != "service-hub") {
        return fail("unexpected service name");
    }

    return 0;
}

int test_invalid_listen_address() {
    const dispatcher::service_hub::Application application;
    std::ostringstream output;

    if (application.run(output, "invalid-address") != 1) {
        return fail("invalid listen address did not fail");
    }

    if (output.str() !=
        "Failed to start Dispatcher Service Hub on invalid-address\n") {
        return fail("unexpected invalid-address diagnostic");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_service_name(); result != 0) {
        return result;
    }

    if (const auto result = test_invalid_listen_address(); result != 0) {
        return result;
    }

    std::cout << "Service Hub application tests passed\n";
    return 0;
}
