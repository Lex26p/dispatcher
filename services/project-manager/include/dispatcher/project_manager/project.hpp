#pragma once

#include <string>

namespace dispatcher::project_manager {

struct Project final {
    std::string id;
    std::string name;
    std::string description;

    friend bool operator==(const Project&, const Project&) = default;
};

struct CreateProjectInput final {
    std::string name;
    std::string description;
};

struct UpdateProjectInput final {
    std::string id;
    std::string name;
    std::string description;
};

}  // namespace dispatcher::project_manager
