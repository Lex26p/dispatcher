#pragma once

#include <dispatcher/runtime/diagnostic_severity.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

namespace dispatcher::runtime
{
    class DiagnosticRecord
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;
        using Metadata = std::unordered_map<std::string, std::string>;

        DiagnosticRecord(
            DiagnosticSeverity severity,
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {},
            TimePoint emitted_at = Clock::now()
        );

        [[nodiscard]] static DiagnosticRecord trace(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] static DiagnosticRecord debug(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] static DiagnosticRecord info(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] static DiagnosticRecord warning(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] static DiagnosticRecord error(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] static DiagnosticRecord critical(
            std::string component,
            std::string code,
            std::string message,
            Metadata metadata = {}
        );

        [[nodiscard]] DiagnosticSeverity severity() const noexcept;

        [[nodiscard]] const std::string& component() const noexcept;

        [[nodiscard]] const std::string& code() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const Metadata& metadata() const noexcept;

        [[nodiscard]] TimePoint emitted_at() const noexcept;

        [[nodiscard]] bool has_component() const noexcept;

        [[nodiscard]] bool has_code() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_metadata() const noexcept;

        [[nodiscard]] bool has_metadata_key(
            const std::string& key
        ) const;

        [[nodiscard]] std::optional<std::string> metadata_value(
            const std::string& key
        ) const;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool operational_note() const noexcept;

        [[nodiscard]] bool warning_or_higher() const noexcept;

        [[nodiscard]] bool error_or_higher() const noexcept;

        [[nodiscard]] bool critical() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

    private:
        DiagnosticSeverity severity_{ DiagnosticSeverity::Unknown };
        std::string component_;
        std::string code_;
        std::string message_;
        Metadata metadata_;
        TimePoint emitted_at_{ Clock::now() };
    };
}