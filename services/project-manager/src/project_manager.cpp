#include "dispatcher/project_manager/project_manager.hpp"

#include <algorithm>
#include <cctype>
#include <random>

namespace dispatcher::project_manager {
namespace {

constexpr std::size_t kMaxProjectNameBytes = 256;
constexpr std::size_t kMaxProjectDescriptionBytes = 4096;
constexpr int kIdGenerationAttempts = 16;

[[nodiscard]] bool contains_non_space(std::string_view value) noexcept {
    return std::any_of(value.begin(), value.end(), [](const unsigned char character) {
        return std::isspace(character) == 0;
    });
}

[[nodiscard]] std::string generate_project_id() {
    constexpr char hex[] = "0123456789abcdef";

    std::random_device random;
    std::string id(32, '0');

    for (char& character : id) {
        character = hex[random() & 0x0fU];
    }

    return id;
}

[[nodiscard]] ProjectManagerError map_read_status(
    const ProjectRepositoryStatus status) noexcept {
    switch (status) {
    case ProjectRepositoryStatus::not_found:
        return ProjectManagerError::not_found;
    case ProjectRepositoryStatus::error:
    case ProjectRepositoryStatus::conflict:
    case ProjectRepositoryStatus::ok:
        return ProjectManagerError::storage_error;
    }

    return ProjectManagerError::storage_error;
}

}  // namespace

ProjectManager::ProjectManager(
    ProjectRepository& repository,
    ProjectIdGenerator id_generator)
    : repository_(repository),
      id_generator_(id_generator ? std::move(id_generator) : generate_project_id) {}

ProjectManagerResult<Project> ProjectManager::create(
    const CreateProjectInput& input) {
    if (const auto error = validate(input.name, input.description);
        error != ProjectManagerError::none) {
        return ProjectManagerResult<Project>::failure(error);
    }

    for (int attempt = 0; attempt < kIdGenerationAttempts; ++attempt) {
        Project project{
            .id = id_generator_(),
            .name = input.name,
            .description = input.description,
        };

        if (project.id.empty()) {
            continue;
        }

        switch (repository_.insert(project)) {
        case ProjectRepositoryStatus::ok:
            return ProjectManagerResult<Project>::success(std::move(project));
        case ProjectRepositoryStatus::conflict:
            continue;
        case ProjectRepositoryStatus::error:
        case ProjectRepositoryStatus::not_found:
            return ProjectManagerResult<Project>::failure(
                ProjectManagerError::storage_error);
        }
    }

    return ProjectManagerResult<Project>::failure(
        ProjectManagerError::id_generation_failed);
}

ProjectManagerResult<std::vector<Project>> ProjectManager::list() const {
    std::vector<Project> projects;
    const auto status = repository_.list(projects);

    if (status != ProjectRepositoryStatus::ok) {
        return ProjectManagerResult<std::vector<Project>>::failure(
            ProjectManagerError::storage_error);
    }

    return ProjectManagerResult<std::vector<Project>>::success(
        std::move(projects));
}

ProjectManagerResult<Project> ProjectManager::get(
    const std::string_view project_id) const {
    Project project;
    const auto status = repository_.find_by_id(project_id, project);

    if (status != ProjectRepositoryStatus::ok) {
        return ProjectManagerResult<Project>::failure(map_read_status(status));
    }

    return ProjectManagerResult<Project>::success(std::move(project));
}

ProjectManagerResult<Project> ProjectManager::update(
    const UpdateProjectInput& input) {
    if (const auto error = validate(input.name, input.description);
        error != ProjectManagerError::none) {
        return ProjectManagerResult<Project>::failure(error);
    }

    Project project;
    const auto find_status = repository_.find_by_id(input.id, project);

    if (find_status != ProjectRepositoryStatus::ok) {
        return ProjectManagerResult<Project>::failure(
            map_read_status(find_status));
    }

    project.name = input.name;
    project.description = input.description;

    const auto update_status = repository_.update(project);

    switch (update_status) {
    case ProjectRepositoryStatus::ok:
        return ProjectManagerResult<Project>::success(std::move(project));
    case ProjectRepositoryStatus::not_found:
        return ProjectManagerResult<Project>::failure(ProjectManagerError::not_found);
    case ProjectRepositoryStatus::error:
    case ProjectRepositoryStatus::conflict:
        return ProjectManagerResult<Project>::failure(
            ProjectManagerError::storage_error);
    }

    return ProjectManagerResult<Project>::failure(ProjectManagerError::storage_error);
}

ProjectManagerError ProjectManager::validate(
    const std::string_view name,
    const std::string_view description) noexcept {
    if (!contains_non_space(name)) {
        return ProjectManagerError::invalid_name;
    }

    if (name.size() > kMaxProjectNameBytes) {
        return ProjectManagerError::name_too_long;
    }

    if (description.size() > kMaxProjectDescriptionBytes) {
        return ProjectManagerError::description_too_long;
    }

    return ProjectManagerError::none;
}

}  // namespace dispatcher::project_manager
