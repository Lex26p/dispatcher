#pragma once

#include <string>

namespace dispatcher::users_access {

struct User final {
    std::string id;
    std::string login;
    std::string display_name;
    bool enabled{true};
};

struct CreateUserInput final {
    std::string login;
    std::string display_name;
    bool enabled{true};
};

}  // namespace dispatcher::users_access
