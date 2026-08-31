#pragma once

#include "dispatcher/device_manager/domain.hpp"

#include <string>
#include <string_view>

struct sqlite3;

namespace dispatcher::device_manager {

enum class MetadataRepositoryStatus {
    ok,
    validation_error,
    storage_error,
};

class SqliteMetadataRepository final {
public:
    explicit SqliteMetadataRepository(std::string_view database_path);
    ~SqliteMetadataRepository();

    SqliteMetadataRepository(const SqliteMetadataRepository&) = delete;
    SqliteMetadataRepository& operator=(const SqliteMetadataRepository&) = delete;

    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& error_message() const noexcept;

    [[nodiscard]] MetadataRepositoryStatus replace_catalog(const DeviceCatalog& catalog);
    [[nodiscard]] MetadataRepositoryStatus load_catalog(DeviceCatalog& catalog) const;

private:
    [[nodiscard]] bool execute(std::string_view sql) const;
    [[nodiscard]] bool initialize_schema();
    void set_error(std::string message) const;

    sqlite3* database_{nullptr};
    bool ready_{false};
    mutable std::string error_message_;
};

}  // namespace dispatcher::device_manager
