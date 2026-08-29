#include "dispatcher/data_hub/application.hpp"

#include <iostream>
#include <sstream>
#include <string_view>

namespace {

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

}  // namespace

int main() {
    using dispatcher::data_hub::Application;

    if (Application::service_name() != "data-hub") {
        return fail("unexpected service name");
    }

    std::ostringstream output;
    const Application application;

    if (application.run(output) != 0) {
        return fail("application returned non-zero exit code");
    }

    if (output.str() != "Dispatcher Data Hub\n") {
        return fail("unexpected application output");
    }

    std::cout << "Data Hub application smoke test passed\n";
    return 0;
}
