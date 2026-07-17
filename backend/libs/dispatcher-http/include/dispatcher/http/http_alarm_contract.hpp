#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dispatcher::http
{
    struct HttpAlarmSeveritySummary
    {
        std::uint64_t critical_count{ 0 };
        std::uint64_t high_count{ 0 };
        std::uint64_t medium_count{ 0 };
        std::uint64_t low_count{ 0 };
        std::uint64_t info_count{ 0 };
    };

    struct HttpAlarmStateSummary
    {
        std::uint64_t active_count{ 0 };
        std::uint64_t acknowledged_count{ 0 };
        std::uint64_t unacknowledged_count{ 0 };
        std::uint64_t shelved_count{ 0 };
        std::uint64_t suppressed_count{ 0 };
        std::uint64_t inhibited_count{ 0 };
    };

    struct HttpAlarmItem
    {
        std::string id{};
        std::string name{};
        std::string tag{};
        std::string severity{};
        std::string state{};
        std::string message{};
        std::string source{ "dispatcher-alarm" };
        bool active{ false };
        bool acknowledged{ false };
        bool shelved{ false };
        bool suppressed{ false };
        bool inhibited{ false };
    };

    struct HttpAlarmSummary
    {
        std::uint64_t schema_version{ 1 };

        std::string status{ "available" };
        std::string endpoint{ "alarms" };
        std::string path{ "/api/v1/alarms" };
        std::string method{ "GET" };
        std::string source{ "dispatcher-http" };

        bool available{ true };

        HttpAlarmSeveritySummary severity{};
        HttpAlarmStateSummary states{};
        std::vector<HttpAlarmItem> items{};
    };

    class HttpAlarmContract
    {
    public:
        [[nodiscard]] static HttpAlarmSummary default_summary();

        [[nodiscard]] static std::string item_to_json(
            const HttpAlarmItem& item
        );

        [[nodiscard]] static std::string items_to_json(
            const std::vector<HttpAlarmItem>& items
        );

        [[nodiscard]] static std::string to_json(
            const HttpAlarmSummary& summary
        );

        [[nodiscard]] static std::string default_json();
    };
}