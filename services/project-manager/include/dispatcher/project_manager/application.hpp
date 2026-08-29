#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::project_manager {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "project-manager";
    }

    int run(std::ostream& output) const;
};

}  // namespace dispatcher::project_manager
