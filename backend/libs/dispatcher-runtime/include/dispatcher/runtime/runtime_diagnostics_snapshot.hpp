#pragma once

#include <dispatcher/runtime/diagnostic_record.hpp>
#include <dispatcher/runtime/diagnostic_severity.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::runtime
{
    class RuntimeDiagnosticsSnapshot
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        explicit RuntimeDiagnosticsSnapshot(
            TimePoint created_at = Clock::now()
        );

        void add_record(
            DiagnosticRecord record
        );

        [[nodiscard]] const std::vector<DiagnosticRecord>& records()
            const noexcept;

        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_records() const noexcept;

        [[nodiscard]] std::size_t record_count() const noexcept;

        [[nodiscard]] std::size_t valid_count() const noexcept;

        [[nodiscard]] std::size_t invalid_count() const noexcept;

        [[nodiscard]] std::size_t trace_count() const noexcept;

        [[nodiscard]] std::size_t debug_count() const noexcept;

        [[nodiscard]] std::size_t info_count() const noexcept;

        [[nodiscard]] std::size_t warning_count() const noexcept;

        [[nodiscard]] std::size_t error_count() const noexcept;

        [[nodiscard]] std::size_t critical_count() const noexcept;

        [[nodiscard]] std::size_t attention_count() const noexcept;

        [[nodiscard]] std::size_t component_count() const;

        [[nodiscard]] bool has_invalid_records() const noexcept;

        [[nodiscard]] bool has_warnings() const noexcept;

        [[nodiscard]] bool has_errors() const noexcept;

        [[nodiscard]] bool has_critical_records() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

        [[nodiscard]] DiagnosticSeverity highest_severity() const noexcept;

        [[nodiscard]] std::vector<DiagnosticRecord> records_for_component(
            const std::string& component
        ) const;

        [[nodiscard]] std::vector<DiagnosticRecord> records_with_severity(
            DiagnosticSeverity severity
        ) const;

        void clear() noexcept;

    private:
        [[nodiscard]] std::size_t count_severity(
            DiagnosticSeverity severity
        ) const noexcept;

        TimePoint created_at_{ Clock::now() };
        std::vector<DiagnosticRecord> records_;
    };
}