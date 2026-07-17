#include <dispatcher/api/runtime_snapshot_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    RuntimeSnapshotApiResult RuntimeSnapshotApiResult::success(
        dispatcher::runtime::DispatcherRuntimeSnapshot snapshot
    )
    {
        return RuntimeSnapshotApiResult(
            ApiResult::success(),
            std::move(snapshot)
        );
    }

    RuntimeSnapshotApiResult RuntimeSnapshotApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return RuntimeSnapshotApiResult(
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

    bool RuntimeSnapshotApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool RuntimeSnapshotApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus RuntimeSnapshotApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& RuntimeSnapshotApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& RuntimeSnapshotApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool RuntimeSnapshotApiResult::has_snapshot() const noexcept
    {
        return snapshot_.has_value();
    }

    const dispatcher::runtime::DispatcherRuntimeSnapshot&
        RuntimeSnapshotApiResult::snapshot() const
    {
        if (!snapshot_.has_value())
        {
            throw std::logic_error(
                "RuntimeSnapshotApiResult does not contain a snapshot"
            );
        }

        return snapshot_.value();
    }

    RuntimeSnapshotApiResult::RuntimeSnapshotApiResult(
        ApiResult result,
        std::optional<dispatcher::runtime::DispatcherRuntimeSnapshot> snapshot
    )
        : result_(std::move(result))
        , snapshot_(std::move(snapshot))
    {
    }
}