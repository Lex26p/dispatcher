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

int test_application_run() {
    const dispatcher::service_hub::Application application;
    std::ostringstream output;

    if (application.run(output) != 0) {
        return fail("application returned a non-zero result");
    }

    if (output.str() != "Dispatcher Service Hub\n") {
        return fail("unexpected application output");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_service_name(); result != 0) {
        return result;
    }

    if (const auto result = test_application_run(); result != 0) {
        return result;
    }

    std::cout << "Service Hub application tests passed\n";
    return 0;
}
