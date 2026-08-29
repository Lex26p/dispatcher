#pragma once

#include "dispatcher/project_manager/project_service_provider.hpp"

#include <iosfwd>
#include <string_view>

namespace dispatcher::project_manager {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "project-manager";
    }

    int run(
        std::ostream& output,
        ProjectServiceProvider& provider,
        std::string_view database_path,
        std::string_view service_hub_address) const;
};

}  // namespace dispatcher::project_manager
