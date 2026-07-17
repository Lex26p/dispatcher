#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace dispatcher::storage::sqlite
{
    struct SqliteConfigurationSnapshotRecord
    {
        std::int64_t id{ 0 };

        std::string snapshot_name{};
        std::string schema_version{ "1" };
        std::string created_at_utc{};
        std::string source{ "dispatcher-config" };
        std::string payload_json{};

        [[nodiscard]] static SqliteConfigurationSnapshotRecord create(
            std::string snapshot_name,
            std::string schema_version,
            std::string created_at_utc,
            std::string payload_json,
            std::string source = "dispatcher-config"
        )
        {
            SqliteConfigurationSnapshotRecord record;

            record.snapshot_name = std::move(
                snapshot_name
            );

            record.schema_version = std::move(
                schema_version
            );

            record.created_at_utc = std::move(
                created_at_utc
            );

            record.payload_json = std::move(
                payload_json
            );

            record.source = std::move(
                source
            );

            return record;
        }
    };
}