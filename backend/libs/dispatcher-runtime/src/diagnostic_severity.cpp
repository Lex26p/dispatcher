#include <dispatcher/runtime/diagnostic_severity.hpp>

namespace dispatcher::runtime
{
    const char* to_string(DiagnosticSeverity severity) noexcept
    {
        switch (severity)
        {
        case DiagnosticSeverity::Unknown:
            return "unknown";

        case DiagnosticSeverity::Trace:
            return "trace";

        case DiagnosticSeverity::Debug:
            return "debug";

        case DiagnosticSeverity::Info:
            return "info";

        case DiagnosticSeverity::Warning:
            return "warning";

        case DiagnosticSeverity::Error:
            return "error";

        case DiagnosticSeverity::Critical:
            return "critical";
        }

        return "unknown";
    }

    std::uint8_t severity_rank(DiagnosticSeverity severity) noexcept
    {
        switch (severity)
        {
        case DiagnosticSeverity::Unknown:
            return 0;

        case DiagnosticSeverity::Trace:
            return 1;

        case DiagnosticSeverity::Debug:
            return 2;

        case DiagnosticSeverity::Info:
            return 3;

        case DiagnosticSeverity::Warning:
            return 4;

        case DiagnosticSeverity::Error:
            return 5;

        case DiagnosticSeverity::Critical:
            return 6;
        }

        return 0;
    }

    bool is_known(DiagnosticSeverity severity) noexcept
    {
        return severity != DiagnosticSeverity::Unknown;
    }

    bool is_operational_note(DiagnosticSeverity severity) noexcept
    {
        return severity == DiagnosticSeverity::Trace
            || severity == DiagnosticSeverity::Debug
            || severity == DiagnosticSeverity::Info;
    }

    bool is_warning_or_higher(DiagnosticSeverity severity) noexcept
    {
        return severity_rank(severity)
            >= severity_rank(DiagnosticSeverity::Warning);
    }

    bool is_error_or_higher(DiagnosticSeverity severity) noexcept
    {
        return severity_rank(severity)
            >= severity_rank(DiagnosticSeverity::Error);
    }

    bool is_critical(DiagnosticSeverity severity) noexcept
    {
        return severity == DiagnosticSeverity::Critical;
    }

    bool requires_attention(DiagnosticSeverity severity) noexcept
    {
        return severity == DiagnosticSeverity::Unknown
            || is_warning_or_higher(severity);
    }
}