#include <dispatcher/api/alarm_operator_snapshot_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    AlarmOperatorSnapshotApiResult AlarmOperatorSnapshotApiResult::success(
        dispatcher::alarm::AlarmOperatorSnapshot snapshot
    )
    {
        return AlarmOperatorSnapshotApiResult(
            ApiResult::success(),
            snapshot
        );
    }

    AlarmOperatorSnapshotApiResult AlarmOperatorSnapshotApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return AlarmOperatorSnapshotApiResult(
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

    bool AlarmOperatorSnapshotApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool AlarmOperatorSnapshotApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus AlarmOperatorSnapshotApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& AlarmOperatorSnapshotApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& AlarmOperatorSnapshotApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool AlarmOperatorSnapshotApiResult::has_snapshot() const noexcept
    {
        return snapshot_.has_value();
    }

    const dispatcher::alarm::AlarmOperatorSnapshot&
        AlarmOperatorSnapshotApiResult::snapshot() const
    {
        if (!snapshot_.has_value())
        {
            throw std::logic_error(
                "AlarmOperatorSnapshotApiResult does not contain a snapshot"
            );
        }

        return snapshot_.value();
    }

    AlarmOperatorSnapshotApiResult::AlarmOperatorSnapshotApiResult(
        ApiResult result,
        std::optional<dispatcher::alarm::AlarmOperatorSnapshot> snapshot
    )
        : result_(std::move(result))
        , snapshot_(std::move(snapshot))
    {
    }
}