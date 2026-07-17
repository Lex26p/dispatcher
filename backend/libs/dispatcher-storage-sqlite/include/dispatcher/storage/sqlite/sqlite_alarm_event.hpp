#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace dispatcher::storage::sqlite
{
    struct SqliteAlarmEvent
    {
        std::int64_t id{ 0 };

        std::string alarm_id{};
        std::string tag_id{};
        std::string event_type{};
        std::string severity{};
        std::string state{};
        std::string message{};
        std::string timestamp_utc{};
        std::string source{ "dispatcher-alarm" };

        std::optional<std::string> operator_id{};
        std::optional<std::string> comment{};

        [[nodiscard]] static SqliteAlarmEvent raised(
            std::string alarm_id,
            std::string tag_id,
            std::string severity,
            std::string message,
            std::string timestamp_utc,
            std::string source = "dispatcher-alarm"
        )
        {
            SqliteAlarmEvent event;

            event.alarm_id = std::move(
                alarm_id
            );

            event.tag_id = std::move(
                tag_id
            );

            event.event_type = "raised";
            event.severity = std::move(
                severity
            );
            event.state = "active";
            event.message = std::move(
                message
            );
            event.timestamp_utc = std::move(
                timestamp_utc
            );
            event.source = std::move(
                source
            );

            return event;
        }

        [[nodiscard]] static SqliteAlarmEvent cleared(
            std::string alarm_id,
            std::string tag_id,
            std::string severity,
            std::string message,
            std::string timestamp_utc,
            std::string source = "dispatcher-alarm"
        )
        {
            SqliteAlarmEvent event;

            event.alarm_id = std::move(
                alarm_id
            );

            event.tag_id = std::move(
                tag_id
            );

            event.event_type = "cleared";
            event.severity = std::move(
                severity
            );
            event.state = "cleared";
            event.message = std::move(
                message
            );
            event.timestamp_utc = std::move(
                timestamp_utc
            );
            event.source = std::move(
                source
            );

            return event;
        }

        [[nodiscard]] static SqliteAlarmEvent acknowledged(
            std::string alarm_id,
            std::string tag_id,
            std::string severity,
            std::string message,
            std::string timestamp_utc,
            std::string operator_id,
            std::string comment = {},
            std::string source = "dispatcher-alarm"
        )
        {
            SqliteAlarmEvent event;

            event.alarm_id = std::move(
                alarm_id
            );

            event.tag_id = std::move(
                tag_id
            );

            event.event_type = "acknowledged";
            event.severity = std::move(
                severity
            );
            event.state = "acknowledged";
            event.message = std::move(
                message
            );
            event.timestamp_utc = std::move(
                timestamp_utc
            );
            event.operator_id = std::move(
                operator_id
            );
            event.comment = std::move(
                comment
            );
            event.source = std::move(
                source
            );

            return event;
        }
    };
}