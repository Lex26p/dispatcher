#pragma once

#include <cstdint>
#include <string>

namespace dispatcher::http
{
    struct HttpRuntimeTelemetrySummary
    {
        bool configured{ true };
        std::uint64_t device_count{ 0 };
        std::uint64_t tag_count{ 0 };
        std::string source{ "in_memory" };
    };

    struct HttpRuntimeAlarmSummary
    {
        bool available{ true };
        std::uint64_t active_count{ 0 };
        std::uint64_t unacknowledged_count{ 0 };
        std::uint64_t shelved_count{ 0 };
        std::uint64_t suppressed_count{ 0 };
    };

    struct HttpRuntimeSummary
    {
        std::uint64_t schema_version{ 1 };

        std::string status{ "available" };
        std::string endpoint{ "runtime" };
        std::string path{ "/api/v1/runtime" };
        std::string method{ "GET" };
        std::string source{ "dispatcher-http" };

        std::string service_name{ "dispatcher" };
        std::string service_component{ "runtime" };
        std::string runtime_state{ "running" };

        bool started{ true };
        bool configured{ true };
        bool accepting_requests{ true };

        HttpRuntimeTelemetrySummary telemetry{};
        HttpRuntimeAlarmSummary alarms{};
    };

    class HttpRuntimeContract
    {
    public:
        [[nodiscard]] static HttpRuntimeSummary default_summary();

        [[nodiscard]] static std::string to_json(
            const HttpRuntimeSummary& summary
        );

        [[nodiscard]] static std::string default_json();
    };
}