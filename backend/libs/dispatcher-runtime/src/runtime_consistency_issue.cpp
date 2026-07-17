#include <dispatcher/runtime/runtime_consistency_issue.hpp>

#include <utility>

namespace dispatcher::runtime
{
    RuntimeConsistencyIssue::RuntimeConsistencyIssue(
        DiagnosticSeverity severity,
        std::string component,
        std::string code,
        std::string message,
        std::string subject,
        std::string expected,
        std::string actual,
        bool blocking,
        Metadata metadata,
        TimePoint detected_at
    )
        : severity_(severity)
        , component_(std::move(component))
        , code_(std::move(code))
        , message_(std::move(message))
        , subject_(std::move(subject))
        , expected_(std::move(expected))
        , actual_(std::move(actual))
        , blocking_(blocking)
        , metadata_(std::move(metadata))
        , detected_at_(detected_at)
    {
    }

    RuntimeConsistencyIssue RuntimeConsistencyIssue::warning(
        std::string component,
        std::string code,
        std::string message,
        std::string subject,
        std::string expected,
        std::string actual,
        bool blocking,
        Metadata metadata
    )
    {
        return RuntimeConsistencyIssue(
            DiagnosticSeverity::Warning,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(subject),
            std::move(expected),
            std::move(actual),
            blocking,
            std::move(metadata)
        );
    }

    RuntimeConsistencyIssue RuntimeConsistencyIssue::error(
        std::string component,
        std::string code,
        std::string message,
        std::string subject,
        std::string expected,
        std::string actual,
        bool blocking,
        Metadata metadata
    )
    {
        return RuntimeConsistencyIssue(
            DiagnosticSeverity::Error,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(subject),
            std::move(expected),
            std::move(actual),
            blocking,
            std::move(metadata)
        );
    }

    RuntimeConsistencyIssue RuntimeConsistencyIssue::critical(
        std::string component,
        std::string code,
        std::string message,
        std::string subject,
        std::string expected,
        std::string actual,
        bool blocking,
        Metadata metadata
    )
    {
        return RuntimeConsistencyIssue(
            DiagnosticSeverity::Critical,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(subject),
            std::move(expected),
            std::move(actual),
            blocking,
            std::move(metadata)
        );
    }

    DiagnosticSeverity RuntimeConsistencyIssue::severity() const noexcept
    {
        return severity_;
    }

    const std::string& RuntimeConsistencyIssue::component() const noexcept
    {
        return component_;
    }

    const std::string& RuntimeConsistencyIssue::code() const noexcept
    {
        return code_;
    }

    const std::string& RuntimeConsistencyIssue::message() const noexcept
    {
        return message_;
    }

    const std::string& RuntimeConsistencyIssue::subject() const noexcept
    {
        return subject_;
    }

    const std::string& RuntimeConsistencyIssue::expected() const noexcept
    {
        return expected_;
    }

    const std::string& RuntimeConsistencyIssue::actual() const noexcept
    {
        return actual_;
    }

    bool RuntimeConsistencyIssue::blocking() const noexcept
    {
        return blocking_;
    }

    bool RuntimeConsistencyIssue::non_blocking() const noexcept
    {
        return !blocking_;
    }

    const RuntimeConsistencyIssue::Metadata&
        RuntimeConsistencyIssue::metadata() const noexcept
    {
        return metadata_;
    }

    RuntimeConsistencyIssue::TimePoint
        RuntimeConsistencyIssue::detected_at() const noexcept
    {
        return detected_at_;
    }

    bool RuntimeConsistencyIssue::has_component() const noexcept
    {
        return !component_.empty();
    }

    bool RuntimeConsistencyIssue::has_code() const noexcept
    {
        return !code_.empty();
    }

    bool RuntimeConsistencyIssue::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool RuntimeConsistencyIssue::has_subject() const noexcept
    {
        return !subject_.empty();
    }

    bool RuntimeConsistencyIssue::has_expected() const noexcept
    {
        return !expected_.empty();
    }

    bool RuntimeConsistencyIssue::has_actual() const noexcept
    {
        return !actual_.empty();
    }

    bool RuntimeConsistencyIssue::has_metadata() const noexcept
    {
        return !metadata_.empty();
    }

    bool RuntimeConsistencyIssue::has_metadata_key(
        const std::string& key
    ) const
    {
        return metadata_.contains(key);
    }

    std::optional<std::string> RuntimeConsistencyIssue::metadata_value(
        const std::string& key
    ) const
    {
        const auto iterator = metadata_.find(key);

        if (iterator == metadata_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    bool RuntimeConsistencyIssue::valid() const noexcept
    {
        return is_known(severity_)
            && warning_or_higher()
            && has_component()
            && has_code()
            && has_message();
    }

    bool RuntimeConsistencyIssue::warning_or_higher() const noexcept
    {
        return is_warning_or_higher(severity_);
    }

    bool RuntimeConsistencyIssue::error_or_higher() const noexcept
    {
        return is_error_or_higher(severity_);
    }

    bool RuntimeConsistencyIssue::critical() const noexcept
    {
        return is_critical(severity_);
    }

    bool RuntimeConsistencyIssue::requires_attention() const noexcept
    {
        return dispatcher::runtime::requires_attention(severity_);
    }

    bool RuntimeConsistencyIssue::blocks_release() const noexcept
    {
        return blocking_
            && (severity_ == DiagnosticSeverity::Error
                || severity_ == DiagnosticSeverity::Critical
                || severity_ == DiagnosticSeverity::Unknown);
    }
}