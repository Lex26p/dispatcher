#pragma once

#include "dispatcher/project_manager/project_repository.hpp"

#include <string>
#include <string_view>

struct sqlite3;

namespace dispatcher::project_manager {

class SqliteProjectRepository final : public ProjectRepository {
public:
    explicit SqliteProjectRepository(std::string_view database_path);
    ~SqliteProjectRepository() override;

    SqliteProjectRepository(const SqliteProjectRepository&) = delete;
    SqliteProjectRepository& operator=(const SqliteProjectRepository&) = delete;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] std::string_view error_message() const noexcept;

    ProjectRepositoryStatus insert(const Project& project) override;
    ProjectRepositoryStatus update(const Project& project) override;

    ProjectRepositoryStatus find_by_id(
        std::string_view project_id,
        Project& project) const override;

    ProjectRepositoryStatus list(std::vector<Project>& projects) const override;

private:
    [[nodiscard]] bool initialize_schema();
    void set_error(std::string message) const;

    sqlite3* database_{nullptr};
    bool ready_{false};
    mutable std::string error_message_;
};

}  // namespace dispatcher::project_manager
