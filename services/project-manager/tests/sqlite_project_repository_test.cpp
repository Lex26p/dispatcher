#include "dispatcher/project_manager/project_manager.hpp"
#include "dispatcher/project_manager/sqlite_project_repository.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {
using dispatcher::project_manager::CreateProjectInput;
using dispatcher::project_manager::Project;
using dispatcher::project_manager::ProjectManager;
using dispatcher::project_manager::ProjectRepositoryStatus;
using dispatcher::project_manager::SqliteProjectRepository;
using dispatcher::project_manager::UpdateProjectInput;

int failures = 0;

void expect(const bool condition, const std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "FAILED: " << message << '\n';
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("dispatcher-project-manager-sqlite-test-" + std::to_string(suffix));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void test_create_update_and_reopen() {
    TemporaryDirectory temporary_directory;
    const auto database_path = temporary_directory.path() / "projects.db";

    {
        SqliteProjectRepository repository{database_path.string()};
        expect(repository.ready(), "new SQLite storage should initialize");
        expect(std::filesystem::exists(database_path), "SQLite storage file should be created");

        ProjectManager manager{repository, [] { return std::string{"project-persisted"}; }};

        const auto created = manager.create(CreateProjectInput{
            .name = "Central Plant",
            .description = "Main engineering project",
        });

        expect(created.ok(), "project should be created in SQLite storage");
        expect(
            created.value.has_value() && created.value->id == "project-persisted",
            "created project should retain generated stable ID");

        const auto updated = manager.update(UpdateProjectInput{
            .id = "project-persisted",
            .name = "Central Plant Updated",
            .description = "Updated description",
        });

        expect(updated.ok(), "project should be updated in SQLite storage");

        const auto duplicate_status = repository.insert(Project{
            .id = "project-persisted",
            .name = "Duplicate identity",
            .description = {},
        });
        expect(
            duplicate_status == ProjectRepositoryStatus::conflict,
            "duplicate project ID should report repository conflict");

        const auto missing_update_status = repository.update(Project{
            .id = "missing-project",
            .name = "Missing",
            .description = {},
        });
        expect(
            missing_update_status == ProjectRepositoryStatus::not_found,
            "updating an unknown project should report not_found");
    }

    {
        SqliteProjectRepository reopened_repository{database_path.string()};
        expect(reopened_repository.ready(), "existing SQLite storage should reopen");

        ProjectManager manager{reopened_repository};
        const auto persisted = manager.get("project-persisted");

        expect(persisted.ok(), "project should survive repository reopen");
        expect(
            persisted.value.has_value() &&
                persisted.value->name == "Central Plant Updated" &&
                persisted.value->description == "Updated description",
            "reopened project should contain the last durable values");

        const auto projects = manager.list();
        expect(projects.ok(), "reopened repository should list projects");
        expect(
            projects.value.has_value() && projects.value->size() == 1,
            "reopened repository should contain exactly the persisted project");
        expect(
            projects.value.has_value() &&
                !projects.value->empty() &&
                projects.value->front().id == "project-persisted",
            "list should preserve the stable project ID across reopen");
    }
}

void test_storage_initialization_errors_are_reported() {
    TemporaryDirectory temporary_directory;

    SqliteProjectRepository empty_path_repository{""};
    expect(!empty_path_repository.ready(), "empty database path should fail initialization");
    expect(
        !empty_path_repository.error_message().empty(),
        "empty database path should expose an initialization error");

    SqliteProjectRepository directory_repository{temporary_directory.path().string()};
    expect(!directory_repository.ready(), "directory path should not open as a SQLite database");
    expect(
        !directory_repository.error_message().empty(),
        "failed SQLite open should expose an initialization error");

    std::vector<Project> projects;
    expect(
        directory_repository.list(projects) == ProjectRepositoryStatus::error,
        "operations on unavailable storage should report repository error");
}

}  // namespace

int main() {
    test_create_update_and_reopen();
    test_storage_initialization_errors_are_reported();

    if (failures != 0) {
        std::cerr << failures << " Project Manager SQLite persistence assertion(s) failed\n";
        return 1;
    }

    std::cout << "Project Manager SQLite persistence tests passed\n";
    return 0;
}
