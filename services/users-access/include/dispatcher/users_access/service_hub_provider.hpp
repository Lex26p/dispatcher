#pragma once

#include "dispatcher/users_access/contract.hpp"
#include "dispatcher/users_access/session.hpp"
#include "dispatcher/users_access/users_access_service_provider.hpp"

#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace dispatcher::users_access {

struct ServiceHubEndpoint final {
    std::string host;
    std::string port;
};

[[nodiscard]] std::optional<ServiceHubEndpoint> parse_service_hub_address(
    std::string_view address);

class ServiceHubProvider final : public UsersAccessServiceProvider {
public:
    static constexpr std::string_view service_address = contract::service_address;

    ServiceHubProvider(
        AuthenticationSessionService& authentication,
        ServiceHubEndpoint endpoint);
    ~ServiceHubProvider() override;

    ServiceHubProvider(const ServiceHubProvider&) = delete;
    ServiceHubProvider& operator=(const ServiceHubProvider&) = delete;

    [[nodiscard]] bool start() override;
    void stop() override;

private:
    void run();

    AuthenticationSessionService& authentication_;
    ServiceHubEndpoint endpoint_;
    std::atomic<bool> stop_requested_{false};
    std::thread worker_;
};

}  // namespace dispatcher::users_access
