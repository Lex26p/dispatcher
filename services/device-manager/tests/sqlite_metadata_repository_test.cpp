#include "dispatcher/device_manager/sqlite_metadata_repository.hpp"

#include <sqlite3.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace dm = dispatcher::device_manager;

namespace {

[[noreturn]] void fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

[[nodiscard]] std::filesystem::path make_temp_dir() {
    const auto base = std::filesystem::temp_directory_path();
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto candidate =
            base / ("dispatcher-device-manager-persistence-test-" +
                    std::to_string(std::rand()));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) {
            return candidate;
        }
    }
    fail("failed to create temporary directory");
}

[[nodiscard]] dm::Metric state_metric(
    std::string id,
    std::optional<std::string> device_id) {
    return dm::Metric{
        .id = std::move(id),
        .device_id = std::move(device_id),
        .name = "State",
        .description = {},
        .value_type = dm::MetricValueType::string,
        .unit = {},
        .writable = false,
        .kind = dm::MetricKind::state,
        .state_metric_id = std::nullopt,
    };
}

[[nodiscard]] dm::Metric working_metric(
    std::string id,
    std::optional<std::string> device_id,
    std::string state_id) {
    return dm::Metric{
        .id = std::move(id),
        .device_id = std::move(device_id),
        .name = "Temperature",
        .description = "Supply temperature",
        .value_type = dm::MetricValueType::floating_point,
        .unit = "degC",
        .writable = true,
        .kind = dm::MetricKind::working,
        .state_metric_id = std::move(state_id),
    };
}

[[nodiscard]] dm::DeviceCatalog sample_catalog() {
    return dm::DeviceCatalog{
        .devices = {
            dm::Device{
                .id = "device-a",
                .name = "Air handler",
                .description = "Main AHU",
                .location = "Plant room",
            },
        },
        .metrics = {
            state_metric("device-a-state", "device-a"),
            working_metric("device-a-temperature", "device-a", "device-a-state"),
            working_metric("standalone-load", std::nullopt, "standalone-state"),
            state_metric("standalone-state", std::nullopt),
        },
        .device_projects = {
            dm::ProjectAssociation{.resource_id = "device-a", .project_id = "project-a"},
            dm::ProjectAssociation{.resource_id = "device-a", .project_id = "project-b"},
        },
        .standalone_metric_projects = {
            dm::ProjectAssociation{.resource_id = "standalone-load", .project_id = "project-a"},
            dm::ProjectAssociation{.resource_id = "standalone-state", .project_id = "project-a"},
        },
    };
}

void test_reopen_preserves_catalog() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "device-manager.db";
    const auto expected = sample_catalog();

    {
        dm::SqliteMetadataRepository repository{database_path.string()};
        expect(repository.ready(), "repository should initialize schema v1");
        expect(
            repository.replace_catalog(expected) == dm::MetadataRepositoryStatus::ok,
            "valid catalog should be stored atomically");
    }

    {
        dm::SqliteMetadataRepository repository{database_path.string()};
        expect(repository.ready(), "repository should reopen existing schema");
        dm::DeviceCatalog loaded;
        expect(
            repository.load_catalog(loaded) == dm::MetadataRepositoryStatus::ok,
            "catalog should load after reopen");
        expect(loaded == expected, "reopened catalog should preserve all metadata");
    }

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

void test_invalid_replace_keeps_previous_catalog() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "device-manager.db";
    const auto expected = sample_catalog();

    dm::SqliteMetadataRepository repository{database_path.string()};
    expect(repository.ready(), "repository should initialize");
    expect(
        repository.replace_catalog(expected) == dm::MetadataRepositoryStatus::ok,
        "initial catalog should store");

    auto invalid = expected;
    invalid.standalone_metric_projects.push_back(
        dm::ProjectAssociation{
            .resource_id = "standalone-load",
            .project_id = "project-b",
        });
    expect(
        repository.replace_catalog(invalid) == dm::MetadataRepositoryStatus::validation_error,
        "invalid catalog should be rejected before mutation");

    dm::DeviceCatalog loaded;
    expect(
        repository.load_catalog(loaded) == dm::MetadataRepositoryStatus::ok,
        "previous catalog should remain readable");
    expect(loaded == expected, "failed replace must not partially change storage");

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

void test_newer_schema_is_rejected() {
    const auto temp_dir = make_temp_dir();
    const auto database_path = temp_dir / "device-manager.db";

    sqlite3* database = nullptr;
    expect(
        sqlite3_open(database_path.string().c_str(), &database) == SQLITE_OK,
        "test should create SQLite database");
    expect(
        sqlite3_exec(database, "PRAGMA user_version = 2;", nullptr, nullptr, nullptr) == SQLITE_OK,
        "test should set newer schema version");
    sqlite3_close(database);

    dm::SqliteMetadataRepository repository{database_path.string()};
    expect(!repository.ready(), "newer schema must not be opened as supported");
    expect(
        repository.error_message().find("newer") != std::string::npos,
        "newer schema rejection should be explicit");

    std::error_code error;
    std::filesystem::remove_all(temp_dir, error);
}

}  // namespace

int main() {
    test_reopen_preserves_catalog();
    test_invalid_replace_keeps_previous_catalog();
    test_newer_schema_is_rejected();
    std::cout << "Device Manager persistence tests passed\n";
    return 0;
}
