#include <dispatcher/alarm/alarm_suppression_reason.hpp>

namespace dispatcher::alarm
{
    const char* to_string(AlarmSuppressionReason reason) noexcept
    {
        switch (reason)
        {
        case AlarmSuppressionReason::Maintenance:
            return "maintenance";

        case AlarmSuppressionReason::Nuisance:
            return "nuisance";

        case AlarmSuppressionReason::Testing:
            return "testing";

        case AlarmSuppressionReason::Commissioning:
            return "commissioning";

        case AlarmSuppressionReason::OperatorDecision:
            return "operator_decision";

        case AlarmSuppressionReason::ExternalInterlock:
            return "external_interlock";

        case AlarmSuppressionReason::Unknown:
            return "unknown";
        }

        return "unknown";
    }

    bool is_known_reason(AlarmSuppressionReason reason) noexcept
    {
        return reason != AlarmSuppressionReason::Unknown;
    }
}