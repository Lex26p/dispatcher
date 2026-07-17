#pragma once

#include <cstdint>

namespace dispatcher::runtime
{
    enum class DiagnosticSeverity
    {
        Unknown,
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    [[nodiscard]] const char* to_string(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] std::uint8_t severity_rank(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool is_known(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool is_operational_note(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool is_warning_or_higher(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool is_error_or_higher(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool is_critical(
        DiagnosticSeverity severity
    ) noexcept;

    [[nodiscard]] bool requires_attention(
        DiagnosticSeverity severity
    ) noexcept;
}