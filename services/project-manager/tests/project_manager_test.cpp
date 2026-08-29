#include "dispatcher/project_manager/project_manager.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
using dispatcher::project_manager::CreateProjectInput;
using dispatcher::project_manager::Project;
using dispatcher::project_manager::ProjectManager;
using dispatcher::project_manager::ProjectManagerError;
using dispatcher::project_manager::ProjectRepository;
using dispatcher::project_manager::ProjectRepositoryStatus;
using dispatcher::project_manager::UpdateProjectInput;

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "FAILED: " << message << '\n';
}

class InMemoryProjectRepository final : public ProjectRepository {
public:
    ProjectRepositoryStatus insert(const Project& project) override {
        if (fail_writes_) {
            return ProjectRepositoryStatus::error;
        }

        if (find(project.id) != projects_.end()) {
            return ProjectRepositoryStatus::conflict;
        }

        projects_.push_back(project);
        return ProjectRepositoryStatus::ok;
    }

    ProjectRepositoryStatus update(const Project& project) override {
        if (fail_writes_) {
            return ProjectRepositoryStatus::error;
        }

        const auto iterator = find(project.id);

        if (iterator == projects_.end()) {
            return ProjectRepositoryStatus::not_found;
        }

        *iterator = project;
        return ProjectRepositoryStatus::ok;
    }

    ProjectRepositoryStatus find_by_id(
        const std::string_view project_id,
        Project& project) const override {
        if (fail_reads_) {
            return ProjectRepositoryStatus::error;
        }

        const auto iterator = find(project_id);

        if (iterator == projects_.end()) {
            return ProjectRepositoryStatus::not_found;
        }

        project = *iterator;
        return ProjectRepositoryStatus::ok;
    }

    ProjectRepositoryStatus list(std::vector<Project>& projects) const override {
        if (fail_reads_) {
            return ProjectRepositoryStatus::error;
        }

        projects = projects_;
        return ProjectRepositoryStatus::ok;
    }

    void fail_reads(bool value) noexcept {
        fail_reads_ = value;
    }

    void fail_writes(bool value) noexcept {
        fail_writes_ = value;
    }

private:
    std::vector<Project>::iterator find(std::string_view project_id) {
        return std::find_if(
            projects_.begin(),
            projects_.end(),
            [project_id](const Project& project) {
                return project.id == project_id;
            });
    }

    std::vector<Project>::const_iterator find(std::string_view project_id) const {
        return std::find_if(
            projects_.cbegin(),
            projects_.cend(),
            [project_id](const Project& project) {
                return project.id == project_id;
            });
    }

    std::vector<Project> projects_;
    bool fail_reads_{false};
    bool fail_writes_{false};
};

void test_create_list_get_update() {
    InMemoryProjectRepository repository;
    std::vector<std::string> ids{"project-a", "project-b"};
    std::size_t next_id = 0;

    ProjectManager manager(repository, [&] {
        return ids.at(next_id++);
    });

    const auto first = manager.create({
        .name = "Котельная",
        .description = "Основной объект",
    });
    const auto second = manager.create({
        .name = "Котельная",
        .description = "Резервный объект",
    });

    expect(first.ok(), "first project should be created");
    expect(second.ok(), "second project should be created");
    expect(first.value->id == "project-a", "first stable id should come from generator");
    expect(second.value->id == "project-b", "duplicate display names should not define identity");

    const auto listed = manager.list();
    expect(listed.ok(), "project list should succeed");
    expect(listed.value->size() == 2, "project list should contain both projects");

    const auto loaded = manager.get("project-a");
    expect(loaded.ok(), "created project should be retrievable by stable id");
    expect(loaded.value->name == "Котельная", "loaded project should keep its name");

    const auto updated = manager.update({
        .id = "project-a",
        .name = "Котельная №1",
        .description = "Переименованный объект",
    });
    expect(updated.ok(), "existing project should update");
    expect(updated.value->id == "project-a", "renaming must not change project id");

    const auto reloaded = manager.get("project-a");
    expect(reloaded.ok(), "updated project should remain retrievable");
    expect(reloaded.value->name == "Котельная №1", "updated name should be stored");
    expect(
        reloaded.value->description == "Переименованный объект",
        "updated description should be stored");
}

void test_validation_and_missing_project() {
    InMemoryProjectRepository repository;
    ProjectManager manager(repository, [] { return std::string("project-a"); });

    const auto empty_name = manager.create({
        .name = " \t\n",
        .description = "invalid",
    });
    expect(
        empty_name.error == ProjectManagerError::invalid_name,
        "whitespace-only project name should be rejected");

    const auto long_name = manager.create({
        .name = std::string(257, 'n'),
        .description = "invalid",
    });
    expect(
        long_name.error == ProjectManagerError::name_too_long,
        "project name over 256 bytes should be rejected");

    const auto long_description = manager.create({
        .name = "Valid",
        .description = std::string(4097, 'd'),
    });
    expect(
        long_description.error == ProjectManagerError::description_too_long,
        "project description over 4096 bytes should be rejected");

    const auto missing = manager.get("missing");
    expect(
        missing.error == ProjectManagerError::not_found,
        "unknown project should return not_found");

    const auto update_missing = manager.update({
        .id = "missing",
        .name = "Still missing",
        .description = "",
    });
    expect(
        update_missing.error == ProjectManagerError::not_found,
        "updating unknown project should return not_found");
}

void test_repository_errors_and_id_conflict_retry() {
    InMemoryProjectRepository repository;
    std::vector<std::string> ids{"duplicate", "duplicate", "unique"};
    std::size_t next_id = 0;

    ProjectManager manager(repository, [&] {
        return ids.at(next_id++);
    });

    const auto existing = manager.create({
        .name = "Existing",
        .description = "",
    });
    expect(existing.ok(), "seed project should be created");

    const auto retried = manager.create({
        .name = "Retried",
        .description = "",
    });
    expect(retried.ok(), "id conflict should trigger a new generated id");
    expect(retried.value->id == "unique", "retry should use the next opaque id");

    repository.fail_reads(true);
    expect(
        manager.list().error == ProjectManagerError::storage_error,
        "repository read failure should map to storage_error");
    repository.fail_reads(false);

    repository.fail_writes(true);
    const auto failed_write = manager.update({
        .id = "unique",
        .name = "Changed",
        .description = "",
    });
    expect(
        failed_write.error == ProjectManagerError::storage_error,
        "repository write failure should map to storage_error");
}

}  // namespace

int main() {
    test_create_list_get_update();
    test_validation_and_missing_project();
    test_repository_errors_and_id_conflict_retry();

    if (failures != 0) {
        std::cerr << failures << " Project Manager test(s) failed\n";
        return 1;
    }

    std::cout << "Project Manager domain/application tests passed\n";
    return 0;
}
