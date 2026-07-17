#pragma once

#include <dispatcher/runtime/diagnostic_severity.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

namespace dispatcher::runtime
{
    class RuntimeConsistencyIssue
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;
        using Metadata = std::unordered_map<std::string, std::string>;

        RuntimeConsistencyIssue(
            DiagnosticSeverity severity,
            std::string component,
            std::string code,
            std::string message,
            std::string subject = {},
            std::string expected = {},
            std::string actual = {},
            bool blocking = true,
            Metadata metadata = {},
            TimePoint detected_at = Clock::now()
        );

        [[nodiscard]] static RuntimeConsistencyIssue warning(
            std::string component,
            std::string code,
            std::string message,
            std::string subject = {},
            std::string expected = {},
            std::string actual = {},
            bool blocking = false,
            Metadata metadata = {}
        );

        [[nodiscard]] static RuntimeConsistencyIssue error(
            std::string component,
            std::string code,
            std::string message,
            std::string subject = {},
            std::string expected = {},
            std::string actual = {},
            bool blocking = true,
            Metadata metadata = {}
        );

        [[nodiscard]] static RuntimeConsistencyIssue critical(
            std::string component,
            std::string code,
            std::string message,
            std::string subject = {},
            std::string expected = {},
            std::string actual = {},
            bool blocking = true,
            Metadata metadata = {}
        );

        [[nodiscard]] DiagnosticSeverity severity() const noexcept;

        [[nodiscard]] const std::string& component() const noexcept;

        [[nodiscard]] const std::string& code() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& subject() const noexcept;

        [[nodiscard]] const std::string& expected() const noexcept;

        [[nodiscard]] const std::string& actual() const noexcept;

        [[nodiscard]] bool blocking() const noexcept;

        [[nodiscard]] bool non_blocking() const noexcept;

        [[nodiscard]] const Metadata& metadata() const noexcept;

        [[nodiscard]] TimePoint detected_at() const noexcept;

        [[nodiscard]] bool has_component() const noexcept;

        [[nodiscard]] bool has_code() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_subject() const noexcept;

        [[nodiscard]] bool has_expected() const noexcept;

        [[nodiscard]] bool has_actual() const noexcept;

        [[nodiscard]] bool has_metadata() const noexcept;

        [[nodiscard]] bool has_metadata_key(
            const std::string& key
        ) const;

        [[nodiscard]] std::optional<std::string> metadata_value(
            const std::string& key
        ) const;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] bool warning_or_higher() const noexcept;

        [[nodiscard]] bool error_or_higher() const noexcept;

        [[nodiscard]] bool critical() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

        [[nodiscard]] bool blocks_release() const noexcept;

    private:
        DiagnosticSeverity severity_{ DiagnosticSeverity::Unknown };
        std::string component_;
        std::string code_;
        std::string message_;
        std::string subject_;
        std::string expected_;
        std::string actual_;
        bool blocking_{ true };
        Metadata metadata_;
        TimePoint detected_at_{ Clock::now() };
    };
}