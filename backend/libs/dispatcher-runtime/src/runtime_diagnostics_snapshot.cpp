#include <dispatcher/runtime/runtime_diagnostics_snapshot.hpp>

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace dispatcher::runtime
{
    RuntimeDiagnosticsSnapshot::RuntimeDiagnosticsSnapshot(
        TimePoint created_at
    )
        : created_at_(created_at)
    {
    }

    void RuntimeDiagnosticsSnapshot::add_record(
        DiagnosticRecord record
    )
    {
        records_.push_back(
            std::move(record)
        );
    }

    const std::vector<DiagnosticRecord>& RuntimeDiagnosticsSnapshot::records()
        const noexcept
    {
        return records_;
    }

    RuntimeDiagnosticsSnapshot::TimePoint
        RuntimeDiagnosticsSnapshot::created_at() const noexcept
    {
        return created_at_;
    }

    bool RuntimeDiagnosticsSnapshot::empty() const noexcept
    {
        return records_.empty();
    }

    bool RuntimeDiagnosticsSnapshot::has_records() const noexcept
    {
        return !empty();
    }

    std::size_t RuntimeDiagnosticsSnapshot::record_count() const noexcept
    {
        return records_.size();
    }

    std::size_t RuntimeDiagnosticsSnapshot::valid_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                records_.begin(),
                records_.end(),
                [](const DiagnosticRecord& record)
                {
                    return record.valid();
                }
            )
            );
    }

    std::size_t RuntimeDiagnosticsSnapshot::invalid_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                records_.begin(),
                records_.end(),
                [](const DiagnosticRecord& record)
                {
                    return !record.valid();
                }
            )
            );
    }

    std::size_t RuntimeDiagnosticsSnapshot::trace_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Trace);
    }

    std::size_t RuntimeDiagnosticsSnapshot::debug_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Debug);
    }

    std::size_t RuntimeDiagnosticsSnapshot::info_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Info);
    }

    std::size_t RuntimeDiagnosticsSnapshot::warning_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Warning);
    }

    std::size_t RuntimeDiagnosticsSnapshot::error_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Error);
    }

    std::size_t RuntimeDiagnosticsSnapshot::critical_count() const noexcept
    {
        return count_severity(DiagnosticSeverity::Critical);
    }

    std::size_t RuntimeDiagnosticsSnapshot::attention_count() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                records_.begin(),
                records_.end(),
                [](const DiagnosticRecord& record)
                {
                    return record.requires_attention();
                }
            )
            );
    }

    std::size_t RuntimeDiagnosticsSnapshot::component_count() const
    {
        std::unordered_set<std::string> components;

        for (const auto& record : records_)
        {
            if (record.has_component())
            {
                components.insert(record.component());
            }
        }

        return components.size();
    }

    bool RuntimeDiagnosticsSnapshot::has_invalid_records() const noexcept
    {
        return invalid_count() > 0;
    }

    bool RuntimeDiagnosticsSnapshot::has_warnings() const noexcept
    {
        return warning_count() > 0;
    }

    bool RuntimeDiagnosticsSnapshot::has_errors() const noexcept
    {
        return error_count() > 0;
    }

    bool RuntimeDiagnosticsSnapshot::has_critical_records() const noexcept
    {
        return critical_count() > 0;
    }

    bool RuntimeDiagnosticsSnapshot::requires_attention() const noexcept
    {
        return attention_count() > 0;
    }

    DiagnosticSeverity RuntimeDiagnosticsSnapshot::highest_severity()
        const noexcept
    {
        if (records_.empty())
        {
            return DiagnosticSeverity::Unknown;
        }

        const auto iterator =
            std::max_element(
                records_.begin(),
                records_.end(),
                [](const DiagnosticRecord& left,
                    const DiagnosticRecord& right)
                {
                    return severity_rank(left.severity())
                        < severity_rank(right.severity());
                }
            );

        if (iterator == records_.end())
        {
            return DiagnosticSeverity::Unknown;
        }

        return iterator->severity();
    }

    std::vector<DiagnosticRecord>
        RuntimeDiagnosticsSnapshot::records_for_component(
            const std::string& component
        ) const
    {
        std::vector<DiagnosticRecord> result;

        for (const auto& record : records_)
        {
            if (record.component() == component)
            {
                result.push_back(record);
            }
        }

        return result;
    }

    std::vector<DiagnosticRecord>
        RuntimeDiagnosticsSnapshot::records_with_severity(
            DiagnosticSeverity severity
        ) const
    {
        std::vector<DiagnosticRecord> result;

        for (const auto& record : records_)
        {
            if (record.severity() == severity)
            {
                result.push_back(record);
            }
        }

        return result;
    }

    void RuntimeDiagnosticsSnapshot::clear() noexcept
    {
        records_.clear();
    }

    std::size_t RuntimeDiagnosticsSnapshot::count_severity(
        DiagnosticSeverity severity
    ) const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(
                records_.begin(),
                records_.end(),
                [&](const DiagnosticRecord& record)
                {
                    return record.severity() == severity;
                }
            )
            );
    }
}