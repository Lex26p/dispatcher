#pragma once

#include <string_view>

namespace dispatcher::domain
{
    enum class HistoryPolicy
    {
        Disabled,
        OnChange,
        OnChangeWithForcedSample,
        EveryPoll,
        CriticalLossless,
        DiagnosticBestEffort,
        LiveOnly
    };

    constexpr std::string_view to_string(HistoryPolicy policy)
    {
        switch (policy)
        {
        case HistoryPolicy::Disabled:
            return "disabled";
        case HistoryPolicy::OnChange:
            return "on_change";
        case HistoryPolicy::OnChangeWithForcedSample:
            return "on_change_with_forced_sample";
        case HistoryPolicy::EveryPoll:
            return "every_poll";
        case HistoryPolicy::CriticalLossless:
            return "critical_lossless";
        case HistoryPolicy::DiagnosticBestEffort:
            return "diagnostic_best_effort";
        case HistoryPolicy::LiveOnly:
            return "live_only";
        }

        return "unknown";
    }
}