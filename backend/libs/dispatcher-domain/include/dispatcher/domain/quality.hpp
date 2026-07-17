#pragma once

#include <string_view>

namespace dispatcher::domain
{
    enum class Quality
    {
        Good,
        Bad,
        Uncertain,
        Stale,
        Timeout,
        CommunicationError,
        Substituted,
        ManualOverride,
        Disabled
    };

    constexpr std::string_view to_string(Quality quality)
    {
        switch (quality)
        {
        case Quality::Good:
            return "good";
        case Quality::Bad:
            return "bad";
        case Quality::Uncertain:
            return "uncertain";
        case Quality::Stale:
            return "stale";
        case Quality::Timeout:
            return "timeout";
        case Quality::CommunicationError:
            return "communication_error";
        case Quality::Substituted:
            return "substituted";
        case Quality::ManualOverride:
            return "manual_override";
        case Quality::Disabled:
            return "disabled";
        }

        return "unknown";
    }
}