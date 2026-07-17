#include <dispatcher/runtime/runtime_consistency_snapshot.hpp>

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace dispatcher::runtime
{
    RuntimeConsistencySnapshot::RuntimeConsistencySnapshot(
        TimePoint created_at
    )
        : created_at_(created_at)
    {
    }

    void RuntimeConsistencySnapshot::add_issue(
        RuntimeConsistencyIssue issue
    )
    {
        issues_.push_back(
            std::move(issue)
        );
    }

    const std::vector<RuntimeConsistencyIssue>&
        RuntimeConsistencySnapshot::issues() const noexcept
    {
        return issues_;
    }

    RuntimeConsistencySnapshot::TimePoint
        RuntimeConsistencySnapshot::created_at() const noexcept
    {
        return created_at_;
    }

    bool RuntimeConsistencySnapshot::empty() const noexcept
    {
        return issues_.empty();
    }

    bool RuntimeConsistencySnapshot::has_issues() const noexcept
    {
        return !empty();
    }

    std::size_t RuntimeConsistencySnapshot::issue_count() const noexcept
    {
        return issues_.size();
    }

    std::size_t RuntimeConsistencySnapshot::valid_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& issue)
                {
                    return issue.valid();
                }
            )
            );
    }

    std::size_t RuntimeConsistencySnapshot::invalid_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& issue)
                {
                    return !issue.valid();
                }
            )
            );
    }

    std::size_t RuntimeConsistencySnapshot::warning_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Warning);
    }

    std::size_t RuntimeConsistencySnapshot::error_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Error);
    }

    std::size_t RuntimeConsistencySnapshot::critical_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Critical);
    }

    std::size_t RuntimeConsistencySnapshot::attention_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& issue)
                {
                    return issue.requires_attention();
                }
            )
            );
    }

    std::size_t RuntimeConsistencySnapshot::blocking_issue_count()
        const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& issue)
                {
                    return issue.blocking();
                }
            )
            );
    }

    std::size_t RuntimeConsistencySnapshot::non_blocking_issue_count()
        const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& issue)
                {
                    return issue.non_blocking();
                }
            )
            );
    }

    std::size_t RuntimeConsistencySnapshot::component_count() const
    {
        std::unordered_set<std::string> components;

        for (const auto& issue : issues_)
        {
            if (issue.has_component())
            {
                components.insert(issue.component());
            }
        }

        return components.size();
    }

    bool RuntimeConsistencySnapshot::has_invalid_issues() const noexcept
    {
        return invalid_count() > 0;
    }

    bool RuntimeConsistencySnapshot::has_warnings() const noexcept
    {
        return warning_count() > 0;
    }

    bool RuntimeConsistencySnapshot::has_errors() const noexcept
    {
        return error_count() > 0;
    }

    bool RuntimeConsistencySnapshot::has_critical_issues() const noexcept
    {
        return critical_count() > 0;
    }

    bool RuntimeConsistencySnapshot::has_blocking_issues() const noexcept
    {
        return blocking_issue_count() > 0;
    }

    bool RuntimeConsistencySnapshot::requires_attention() const noexcept
    {
        return has_invalid_issues()
            || attention_count() > 0;
    }

    bool RuntimeConsistencySnapshot::blocks_release() const noexcept
    {
        return std::any_of(
            issues_.begin(),
            issues_.end(),
            [](const RuntimeConsistencyIssue& issue)
            {
                return issue.blocks_release();
            }
        );
    }

    RuntimeConsistencyStatus RuntimeConsistencySnapshot::status()
        const noexcept
    {
        if (has_invalid_issues())
        {
            return RuntimeConsistencyStatus::Unknown;
        }

        if (blocks_release())
        {
            return RuntimeConsistencyStatus::Inconsistent;
        }

        return RuntimeConsistencyStatus::Consistent;
    }

    bool RuntimeConsistencySnapshot::consistent() const noexcept
    {
        return status() == RuntimeConsistencyStatus::Consistent;
    }

    bool RuntimeConsistencySnapshot::inconsistent() const noexcept
    {
        return status() == RuntimeConsistencyStatus::Inconsistent;
    }

    DiagnosticSeverity RuntimeConsistencySnapshot::highest_severity()
        const noexcept
    {
        if (issues_.empty())
        {
            return DiagnosticSeverity::Unknown;
        }

        const auto iterator =
            std::max_element(
                issues_.begin(),
                issues_.end(),
                [](const RuntimeConsistencyIssue& left,
                    const RuntimeConsistencyIssue& right)
                {
                    return severity_rank(left.severity())
                        < severity_rank(right.severity());
                }
            );

        if (iterator == issues_.end())
        {
            return DiagnosticSeverity::Unknown;
        }

        return iterator->severity();
    }

    std::vector<RuntimeConsistencyIssue>
        RuntimeConsistencySnapshot::issues_for_component(
            const std::string& component
        ) const
    {
        std::vector<RuntimeConsistencyIssue> result;

        for (const auto& issue : issues_)
        {
            if (issue.component() == component)
            {
                result.push_back(issue);
            }
        }

        return result;
    }

    std::vector<RuntimeConsistencyIssue>
        RuntimeConsistencySnapshot::issues_with_severity(
            DiagnosticSeverity severity
        ) const
    {
        std::vector<RuntimeConsistencyIssue> result;

        for (const auto& issue : issues_)
        {
            if (issue.severity() == severity)
            {
                result.push_back(issue);
            }
        }

        return result;
    }

    void RuntimeConsistencySnapshot::clear() noexcept
    {
        issues_.clear();
    }

    std::size_t RuntimeConsistencySnapshot::count_severity(
        DiagnosticSeverity severity
    ) const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                issues_.begin(),
                issues_.end(),
                [&](const RuntimeConsistencyIssue& issue)
                {
                    return issue.severity() == severity;
                }
            )
            );
    }
}