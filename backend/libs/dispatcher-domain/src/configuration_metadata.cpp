#include <dispatcher/domain/configuration_metadata.hpp>

#include <utility>

namespace dispatcher::domain
{
    ConfigurationMetadata::ConfigurationMetadata(
        std::uint64_t config_version,
        ConfigurationStatus status,
        std::string description,
        TimePoint created_at
    )
        : config_version_(config_version)
        , status_(status)
        , description_(std::move(description))
        , created_at_(created_at)
    {
    }

    ConfigurationMetadata ConfigurationMetadata::draft(
        std::uint64_t config_version,
        std::string description
    )
    {
        return ConfigurationMetadata(
            config_version,
            ConfigurationStatus::Draft,
            std::move(description),
            Clock::now()
        );
    }

    ConfigurationMetadata ConfigurationMetadata::published(
        std::uint64_t config_version,
        std::string description
    )
    {
        return ConfigurationMetadata(
            config_version,
            ConfigurationStatus::Published,
            std::move(description),
            Clock::now()
        );
    }

    std::uint64_t ConfigurationMetadata::config_version() const noexcept
    {
        return config_version_;
    }

    ConfigurationStatus ConfigurationMetadata::status() const noexcept
    {
        return status_;
    }

    const std::string& ConfigurationMetadata::description() const noexcept
    {
        return description_;
    }

    ConfigurationMetadata::TimePoint ConfigurationMetadata::created_at() const noexcept
    {
        return created_at_;
    }

    bool ConfigurationMetadata::is_draft() const noexcept
    {
        return status_ == ConfigurationStatus::Draft;
    }

    bool ConfigurationMetadata::is_published() const noexcept
    {
        return status_ == ConfigurationStatus::Published;
    }
}