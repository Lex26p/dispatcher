#pragma once

#include "dispatcher/users_access/users_access_service_provider.hpp"

#include <iosfwd>
#include <string_view>

namespace dispatcher::users_access {

class Application final {
public:
    [[nodiscard]] static constexpr std::string_view service_name() noexcept {
        return "users-access";
    }

    int run(
        std::ostream& output,
        UsersAccessServiceProvider& provider,
        std::string_view database_path,
        std::string_view service_hub_address) const;
};

}  // namespace dispatcher::users_access
