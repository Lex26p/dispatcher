#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace dispatcher::storage::sqlite
{
    enum class SqliteHistoryValueType
    {
        numeric,
        text,
        boolean
    };

    struct SqliteHistorySample
    {
        std::int64_t id{ 0 };

        std::string tag_id{};
        std::string timestamp_utc{};

        SqliteHistoryValueType value_type{
            SqliteHistoryValueType::numeric
        };

        std::optional<double> numeric_value{};
        std::optional<std::string> text_value{};
        std::optional<bool> bool_value{};

        std::string quality{ "good" };
        std::string source{ "dispatcher" };

        [[nodiscard]] static SqliteHistorySample numeric(
            std::string tag_id,
            std::string timestamp_utc,
            double value,
            std::string quality = "good",
            std::string source = "dispatcher"
        )
        {
            SqliteHistorySample sample;

            sample.tag_id = std::move(
                tag_id
            );

            sample.timestamp_utc = std::move(
                timestamp_utc
            );

            sample.value_type = SqliteHistoryValueType::numeric;
            sample.numeric_value = value;
            sample.quality = std::move(
                quality
            );
            sample.source = std::move(
                source
            );

            return sample;
        }

        [[nodiscard]] static SqliteHistorySample text(
            std::string tag_id,
            std::string timestamp_utc,
            std::string value,
            std::string quality = "good",
            std::string source = "dispatcher"
        )
        {
            SqliteHistorySample sample;

            sample.tag_id = std::move(
                tag_id
            );

            sample.timestamp_utc = std::move(
                timestamp_utc
            );

            sample.value_type = SqliteHistoryValueType::text;
            sample.text_value = std::move(
                value
            );
            sample.quality = std::move(
                quality
            );
            sample.source = std::move(
                source
            );

            return sample;
        }

        [[nodiscard]] static SqliteHistorySample boolean(
            std::string tag_id,
            std::string timestamp_utc,
            bool value,
            std::string quality = "good",
            std::string source = "dispatcher"
        )
        {
            SqliteHistorySample sample;

            sample.tag_id = std::move(
                tag_id
            );

            sample.timestamp_utc = std::move(
                timestamp_utc
            );

            sample.value_type = SqliteHistoryValueType::boolean;
            sample.bool_value = value;
            sample.quality = std::move(
                quality
            );
            sample.source = std::move(
                source
            );

            return sample;
        }
    };
}