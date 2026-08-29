#pragma once

#include "dispatcher/project_manager/project.hpp"
#include "dispatcher/project_manager/project_repository.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dispatcher::project_manager {

enum class ProjectManagerError {
    none,
    invalid_name,
    name_too_long,
    description_too_long,
    not_found,
    storage_error,
    id_generation_failed,
};

template <typename T>
struct ProjectManagerResult final {
    std::optional<T> value;
    ProjectManagerError error{ProjectManagerError::none};

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && error == ProjectManagerError::none;
    }

    [[nodiscard]] static ProjectManagerResult success(T result) {
        return ProjectManagerResult{std::move(result), ProjectManagerError::none};
    }

    [[nodiscard]] static ProjectManagerResult failure(ProjectManagerError error) {
        return ProjectManagerResult{std::nullopt, error};
    }
};

using ProjectIdGenerator = std::function<std::string()>;

class ProjectManager final {
public:
    explicit ProjectManager(
        ProjectRepository& repository,
        ProjectIdGenerator id_generator = {});

    [[nodiscard]] ProjectManagerResult<Project> create(
        const CreateProjectInput& input);

    [[nodiscard]] ProjectManagerResult<std::vector<Project>> list() const;

    [[nodiscard]] ProjectManagerResult<Project> get(
        std::string_view project_id) const;

    [[nodiscard]] ProjectManagerResult<Project> update(
        const UpdateProjectInput& input);

private:
    [[nodiscard]] static ProjectManagerError validate(
        std::string_view name,
        std::string_view description) noexcept;

    ProjectRepository& repository_;
    ProjectIdGenerator id_generator_;
};

}  // namespace dispatcher::project_manager
