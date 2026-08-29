#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace dispatcher::service_hub {

class ServiceHubServer final {
public:
    explicit ServiceHubServer(std::string listen_address);
    ~ServiceHubServer();

    ServiceHubServer(const ServiceHubServer&) = delete;
    ServiceHubServer& operator=(const ServiceHubServer&) = delete;
    ServiceHubServer(ServiceHubServer&&) = delete;
    ServiceHubServer& operator=(ServiceHubServer&&) = delete;

    [[nodiscard]] bool start();
    void shutdown();

    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] int bound_port() const noexcept;
    [[nodiscard]] std::string_view listen_address() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace dispatcher::service_hub
