#include "dispatcher/service_hub/provider_registry.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

using dispatcher::service_hub::ProviderConnectionId;
using dispatcher::service_hub::ProviderRegistry;

int fail(std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    return 1;
}

int test_service_address_validation() {
    const std::string valid_128(128, 'a');
    const std::string invalid_129(129, 'a');

    const std::string_view valid_addresses[] = {
        "a",
        "device-manager",
        "project_manager",
        "test.echo",
        "service-2.v1",
        valid_128,
    };

    for (const auto address : valid_addresses) {
        if (!ProviderRegistry::is_valid_service_address(address)) {
            return fail("valid service address was rejected");
        }
    }

    const std::string_view invalid_addresses[] = {
        "",
        ".test",
        "-test",
        "_test",
        "Test",
        "test/Echo",
        "test echo",
        "тест",
        invalid_129,
    };

    for (const auto address : invalid_addresses) {
        if (ProviderRegistry::is_valid_service_address(address)) {
            return fail("invalid service address was accepted");
        }
    }

    return 0;
}

int test_registration_and_lookup() {
    ProviderRegistry registry;

    constexpr ProviderConnectionId first_connection = 101;
    constexpr ProviderConnectionId second_connection = 202;

    if (registry.register_provider(first_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::registered) {
        return fail("valid provider registration failed");
    }

    if (registry.size() != 1) {
        return fail("registry size is invalid after registration");
    }

    const auto provider = registry.find_provider("test.echo");

    if (!provider.has_value() || *provider != first_connection) {
        return fail("service route does not resolve to its provider");
    }

    const auto service = registry.find_service(first_connection);

    if (!service.has_value() || *service != "test.echo") {
        return fail("connection does not resolve to its service");
    }

    if (registry.find_provider("unknown").has_value()) {
        return fail("unknown service unexpectedly resolved");
    }

    if (registry.find_service(second_connection).has_value()) {
        return fail("unknown connection unexpectedly resolved");
    }

    return 0;
}

int test_registration_conflicts() {
    ProviderRegistry registry;

    constexpr ProviderConnectionId first_connection = 101;
    constexpr ProviderConnectionId second_connection = 202;

    if (registry.register_provider(first_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::registered) {
        return fail("initial provider registration failed");
    }

    if (registry.register_provider(second_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::service_in_use) {
        return fail("duplicate service registration was not rejected");
    }

    if (registry.register_provider(first_connection, "test.other") !=
        ProviderRegistry::RegisterResult::connection_already_registered) {
        return fail("second service on one connection was not rejected");
    }

    if (registry.register_provider(first_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::connection_already_registered) {
        return fail("repeated registration on one connection was not rejected");
    }

    const auto provider = registry.find_provider("test.echo");

    if (!provider.has_value() || *provider != first_connection) {
        return fail("conflict changed the existing service owner");
    }

    if (registry.size() != 1) {
        return fail("conflicts changed registry size");
    }

    return 0;
}

int test_unregister_and_reconnect() {
    ProviderRegistry registry;

    constexpr ProviderConnectionId first_connection = 101;
    constexpr ProviderConnectionId second_connection = 202;

    if (registry.register_provider(first_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::registered) {
        return fail("initial provider registration failed");
    }

    if (!registry.unregister_provider(first_connection)) {
        return fail("registered provider was not removed");
    }

    if (registry.find_provider("test.echo").has_value() ||
        registry.find_service(first_connection).has_value() ||
        registry.size() != 0) {
        return fail("provider route remained after disconnect");
    }

    if (registry.unregister_provider(first_connection)) {
        return fail("unknown provider removal unexpectedly succeeded");
    }

    if (registry.register_provider(second_connection, "test.echo") !=
        ProviderRegistry::RegisterResult::registered) {
        return fail("provider could not re-register after reconnect");
    }

    const auto provider = registry.find_provider("test.echo");

    if (!provider.has_value() || *provider != second_connection) {
        return fail("reconnected provider did not own the route");
    }

    return 0;
}

int test_invalid_registration() {
    ProviderRegistry registry;

    if (registry.register_provider(101, "Invalid.Service") !=
        ProviderRegistry::RegisterResult::invalid_service) {
        return fail("invalid service address registration was accepted");
    }

    if (registry.size() != 0) {
        return fail("invalid registration changed the registry");
    }

    return 0;
}

}  // namespace

int main() {
    if (const auto result = test_service_address_validation(); result != 0) {
        return result;
    }

    if (const auto result = test_registration_and_lookup(); result != 0) {
        return result;
    }

    if (const auto result = test_registration_conflicts(); result != 0) {
        return result;
    }

    if (const auto result = test_unregister_and_reconnect(); result != 0) {
        return result;
    }

    if (const auto result = test_invalid_registration(); result != 0) {
        return result;
    }

    std::cout << "Service Hub provider registry tests passed\n";
    return 0;
}
