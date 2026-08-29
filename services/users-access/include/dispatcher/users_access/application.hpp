#pragma once

#include <iosfwd>
#include <string_view>

namespace dispatcher::users_access {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "users-access";
    }

    int run(std::ostream& output) const;
};

}  // namespace dispatcher::users_access
