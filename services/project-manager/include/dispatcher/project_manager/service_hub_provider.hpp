#pragma once

#include "dispatcher/project_manager/project_manager.hpp"
#include "dispatcher/project_manager/project_service_provider.hpp"

#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace dispatcher::project_manager {

struct ServiceHubEndpoint final {
    std::string host;
    std::string port;
};

[[nodiscard]] std::optional<ServiceHubEndpoint> parse_service_hub_address(
    std::string_view address);

class ServiceHubProvider final : public ProjectServiceProvider {
public:
    static constexpr std::string_view service_address = "project-manager.v1";

    ServiceHubProvider(ProjectManager& project_manager, ServiceHubEndpoint endpoint);
    ~ServiceHubProvider() override;

    ServiceHubProvider(const ServiceHubProvider&) = delete;
    ServiceHubProvider& operator=(const ServiceHubProvider&) = delete;

    [[nodiscard]] bool start() override;
    void stop() override;

private:
    void run();

    ProjectManager& project_manager_;
    ServiceHubEndpoint endpoint_;
    std::atomic<bool> stop_requested_{false};
    std::thread worker_;
};

}  // namespace dispatcher::project_manager
