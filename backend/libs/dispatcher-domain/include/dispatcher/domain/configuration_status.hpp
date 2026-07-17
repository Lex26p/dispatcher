#pragma once

#include <string_view>

namespace dispatcher::domain
{
    enum class ConfigurationStatus
    {
        Draft,
        Published
    };

    constexpr std::string_view to_string(ConfigurationStatus status)
    {
        switch (status)
        {
        case ConfigurationStatus::Draft:
            return "draft";
        case ConfigurationStatus::Published:
            return "published";
        }

        return "unknown";
    }
}