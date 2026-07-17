#pragma once

#include <dispatcher/api/alarm_acknowledgement_api_result.hpp>
#include <dispatcher/api/alarm_acknowledgement_request.hpp>
#include <dispatcher/api/alarm_operator_snapshot_api_result.hpp>
#include <dispatcher/api/unacknowledged_alarms_api_result.hpp>

namespace dispatcher::api
{
    class AlarmApi
    {
    public:
        virtual ~AlarmApi() = default;

        [[nodiscard]] virtual AlarmOperatorSnapshotApiResult
            operator_snapshot() const = 0;

        [[nodiscard]] virtual UnacknowledgedAlarmsApiResult
            unacknowledged_alarms() const = 0;

        [[nodiscard]] virtual AlarmAcknowledgementApiResult acknowledge(
            const AlarmAcknowledgementRequest& request
        ) = 0;
    };
}