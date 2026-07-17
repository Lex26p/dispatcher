#include <dispatcher/runtime/diagnostic_record.hpp>

#include <utility>

namespace dispatcher::runtime
{
    DiagnosticRecord::DiagnosticRecord(
        DiagnosticSeverity severity,
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata,
        TimePoint emitted_at
    )
        : severity_(severity)
        , component_(std::move(component))
        , code_(std::move(code))
        , message_(std::move(message))
        , metadata_(std::move(metadata))
        , emitted_at_(emitted_at)
    {
    }

    DiagnosticRecord DiagnosticRecord::trace(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Trace,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticRecord DiagnosticRecord::debug(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Debug,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticRecord DiagnosticRecord::info(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Info,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticRecord DiagnosticRecord::warning(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Warning,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticRecord DiagnosticRecord::error(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Error,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticRecord DiagnosticRecord::critical(
        std::string component,
        std::string code,
        std::string message,
        Metadata metadata
    )
    {
        return DiagnosticRecord(
            DiagnosticSeverity::Critical,
            std::move(component),
            std::move(code),
            std::move(message),
            std::move(metadata)
        );
    }

    DiagnosticSeverity DiagnosticRecord::severity() const noexcept
    {
        return severity_;
    }

    const std::string& DiagnosticRecord::component() const noexcept
    {
        return component_;
    }

    const std::string& DiagnosticRecord::code() const noexcept
    {
        return code_;
    }

    const std::string& DiagnosticRecord::message() const noexcept
    {
        return message_;
    }

    const DiagnosticRecord::Metadata& DiagnosticRecord::metadata()
        const noexcept
    {
        return metadata_;
    }

    DiagnosticRecord::TimePoint DiagnosticRecord::emitted_at() const noexcept
    {
        return emitted_at_;
    }

    bool DiagnosticRecord::has_component() const noexcept
    {
        return !component_.empty();
    }

    bool DiagnosticRecord::has_code() const noexcept
    {
        return !code_.empty();
    }

    bool DiagnosticRecord::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool DiagnosticRecord::has_metadata() const noexcept
    {
        return !metadata_.empty();
    }

    bool DiagnosticRecord::has_metadata_key(
        const std::string& key
    ) const
    {
        return metadata_.contains(key);
    }

    std::optional<std::string> DiagnosticRecord::metadata_value(
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

    bool DiagnosticRecord::valid() const noexcept
    {
        return is_known(severity_)
            && has_component()
            && has_code()
            && has_message();
    }

    bool DiagnosticRecord::operational_note() const noexcept
    {
        return is_operational_note(severity_);
    }

    bool DiagnosticRecord::warning_or_higher() const noexcept
    {
        return is_warning_or_higher(severity_);
    }

    bool DiagnosticRecord::error_or_higher() const noexcept
    {
        return is_error_or_higher(severity_);
    }

    bool DiagnosticRecord::critical() const noexcept
    {
        return is_critical(severity_);
    }

    bool DiagnosticRecord::requires_attention() const noexcept
    {
        return dispatcher::runtime::requires_attention(severity_);
    }
}