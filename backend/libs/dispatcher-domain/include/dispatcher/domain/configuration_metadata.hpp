#pragma once

#include <dispatcher/domain/configuration_status.hpp>

#include <chrono>
#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class ConfigurationMetadata
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        ConfigurationMetadata(
            std::uint64_t config_version,
            ConfigurationStatus status,
            std::string description,
            TimePoint created_at
        );

        static ConfigurationMetadata draft(
            std::uint64_t config_version,
            std::string description = {}
        );

        static ConfigurationMetadata published(
            std::uint64_t config_version,
            std::string description = {}
        );

        [[nodiscard]] std::uint64_t config_version() const noexcept;
        [[nodiscard]] ConfigurationStatus status() const noexcept;
        [[nodiscard]] const std::string& description() const noexcept;
        [[nodiscard]] TimePoint created_at() const noexcept;

        [[nodiscard]] bool is_draft() const noexcept;
        [[nodiscard]] bool is_published() const noexcept;

    private:
        std::uint64_t config_version_;
        ConfigurationStatus status_;
        std::string description_;
        TimePoint created_at_;
    };
}