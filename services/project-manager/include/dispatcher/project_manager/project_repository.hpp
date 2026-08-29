#pragma once

#include "dispatcher/project_manager/project.hpp"

#include <string_view>
#include <vector>

namespace dispatcher::project_manager {

enum class ProjectRepositoryStatus {
    ok,
    not_found,
    conflict,
    error,
};

class ProjectRepository {
public:
    virtual ~ProjectRepository() = default;

    virtual ProjectRepositoryStatus insert(const Project& project) = 0;
    virtual ProjectRepositoryStatus update(const Project& project) = 0;

    virtual ProjectRepositoryStatus find_by_id(
        std::string_view project_id,
        Project& project) const = 0;

    virtual ProjectRepositoryStatus list(std::vector<Project>& projects) const = 0;
};

}  // namespace dispatcher::project_manager
