#pragma once

#include <dispatcher/runtime/diagnostic_severity.hpp>
#include <dispatcher/runtime/runtime_consistency_issue.hpp>
#include <dispatcher/runtime/runtime_consistency_status.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::runtime
{
    class RuntimeConsistencySnapshot
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        explicit RuntimeConsistencySnapshot(
            TimePoint created_at = Clock::now()
        );

        void add_issue(
            RuntimeConsistencyIssue issue
        );

        [[nodiscard]] const std::vector<RuntimeConsistencyIssue>& issues()
            const noexcept;

        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_issues() const noexcept;

        [[nodiscard]] std::size_t issue_count() const noexcept;

        [[nodiscard]] std::size_t valid_count() const noexcept;

        [[nodiscard]] std::size_t invalid_count() const noexcept;

        [[nodiscard]] std::size_t warning_count() const noexcept;

        [[nodiscard]] std::size_t error_count() const noexcept;

        [[nodiscard]] std::size_t critical_count() const noexcept;

        [[nodiscard]] std::size_t attention_count() const noexcept;

        [[nodiscard]] std::size_t blocking_issue_count() const noexcept;

        [[nodiscard]] std::size_t non_blocking_issue_count() const noexcept;

        [[nodiscard]] std::size_t component_count() const;

        [[nodiscard]] bool has_invalid_issues() const noexcept;

        [[nodiscard]] bool has_warnings() const noexcept;

        [[nodiscard]] bool has_errors() const noexcept;

        [[nodiscard]] bool has_critical_issues() const noexcept;

        [[nodiscard]] bool has_blocking_issues() const noexcept;

        [[nodiscard]] bool requires_attention() const noexcept;

        [[nodiscard]] bool blocks_release() const noexcept;

        [[nodiscard]] RuntimeConsistencyStatus status() const noexcept;

        [[nodiscard]] bool consistent() const noexcept;

        [[nodiscard]] bool inconsistent() const noexcept;

        [[nodiscard]] DiagnosticSeverity highest_severity() const noexcept;

        [[nodiscard]] std::vector<RuntimeConsistencyIssue>
            issues_for_component(
                const std::string& component
            ) const;

        [[nodiscard]] std::vector<RuntimeConsistencyIssue>
            issues_with_severity(
                DiagnosticSeverity severity
            ) const;

        void clear() noexcept;

    private:
        [[nodiscard]] std::size_t count_severity(
            DiagnosticSeverity severity
        ) const noexcept;

        TimePoint created_at_{ Clock::now() };
        std::vector<RuntimeConsistencyIssue> issues_;
    };
}