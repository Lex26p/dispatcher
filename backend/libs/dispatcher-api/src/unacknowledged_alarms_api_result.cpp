#include <dispatcher/api/unacknowledged_alarms_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    UnacknowledgedAlarmsApiResult
        UnacknowledgedAlarmsApiResult::success(
            UnacknowledgedAlarmList alarms
        )
    {
        return UnacknowledgedAlarmsApiResult(
            ApiResult::success(),
            std::move(alarms)
        );
    }

    UnacknowledgedAlarmsApiResult
        UnacknowledgedAlarmsApiResult::failure(
            ApiStatus status,
            std::string operation,
            std::string resource,
            std::string field,
            std::string message
        )
    {
        return UnacknowledgedAlarmsApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool UnacknowledgedAlarmsApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool UnacknowledgedAlarmsApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus UnacknowledgedAlarmsApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& UnacknowledgedAlarmsApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& UnacknowledgedAlarmsApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool UnacknowledgedAlarmsApiResult::has_alarms() const noexcept
    {
        return alarms_.has_value();
    }

    const UnacknowledgedAlarmList&
        UnacknowledgedAlarmsApiResult::alarms() const
    {
        if (!alarms_.has_value())
        {
            throw std::logic_error(
                "UnacknowledgedAlarmsApiResult does not contain alarms"
            );
        }

        return alarms_.value();
    }

    std::size_t UnacknowledgedAlarmsApiResult::alarm_count() const noexcept
    {
        if (!alarms_.has_value())
        {
            return 0;
        }

        return alarms_->size();
    }

    bool UnacknowledgedAlarmsApiResult::empty() const noexcept
    {
        return alarm_count() == 0;
    }

    UnacknowledgedAlarmsApiResult::UnacknowledgedAlarmsApiResult(
        ApiResult result,
        std::optional<UnacknowledgedAlarmList> alarms
    )
        : result_(std::move(result))
        , alarms_(std::move(alarms))
    {
    }
}