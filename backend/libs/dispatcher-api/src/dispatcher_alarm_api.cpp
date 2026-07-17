#include <dispatcher/api/dispatcher_alarm_api.hpp>

#include <dispatcher/api/api_status_mapping.hpp>

namespace dispatcher::api
{
    DispatcherAlarmApi::DispatcherAlarmApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
    {
    }

    AlarmOperatorSnapshotApiResult DispatcherAlarmApi::operator_snapshot()
        const
    {
        if (runtime_ == nullptr)
        {
            return AlarmOperatorSnapshotApiResult::failure(
                ApiStatus::InternalError,
                "alarm.operator_snapshot",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        return AlarmOperatorSnapshotApiResult::success(
            runtime_->alarm_operator_snapshot()
        );
    }

    UnacknowledgedAlarmsApiResult DispatcherAlarmApi::unacknowledged_alarms()
        const
    {
        if (runtime_ == nullptr)
        {
            return UnacknowledgedAlarmsApiResult::failure(
                ApiStatus::InternalError,
                "alarm.unacknowledged_alarms",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        return UnacknowledgedAlarmsApiResult::success(
            runtime_->unacknowledged_alarms()
        );
    }

    AlarmAcknowledgementApiResult DispatcherAlarmApi::acknowledge(
        const AlarmAcknowledgementRequest& request
    )
    {
        if (runtime_ == nullptr)
        {
            return AlarmAcknowledgementApiResult::failure(
                ApiStatus::InternalError,
                "alarm.acknowledge",
                request.alarm_id.value(),
                {},
                "runtime instance is not available"
            );
        }

        const auto acknowledgement = runtime_->acknowledge_alarm(
            request.to_command()
        );

        const auto api_status =
            map_alarm_acknowledgement_status_to_api_status(
                acknowledgement.status()
            );

        if (api_status == ApiStatus::Success)
        {
            return AlarmAcknowledgementApiResult::success(
                acknowledgement
            );
        }

        return AlarmAcknowledgementApiResult::failure(
            api_status,
            "alarm.acknowledge",
            request.alarm_id.value(),
            {},
            alarm_acknowledgement_status_message(acknowledgement.status())
        );
    }

    dispatcher::runtime::DispatcherRuntime&
        DispatcherAlarmApi::runtime() noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime&
        DispatcherAlarmApi::runtime() const noexcept
    {
        return *runtime_;
    }
}