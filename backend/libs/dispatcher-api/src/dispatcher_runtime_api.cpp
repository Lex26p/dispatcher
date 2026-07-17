#include <dispatcher/api/dispatcher_runtime_api.hpp>

namespace dispatcher::api
{
    DispatcherRuntimeApi::DispatcherRuntimeApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
    {
    }

    RuntimeSnapshotApiResult DispatcherRuntimeApi::runtime_snapshot() const
    {
        if (runtime_ == nullptr)
        {
            return RuntimeSnapshotApiResult::failure(
                ApiStatus::InternalError,
                "runtime.snapshot",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        return RuntimeSnapshotApiResult::success(
            runtime_->runtime_snapshot()
        );
    }

    AlarmOperatorSnapshotApiResult
        DispatcherRuntimeApi::alarm_operator_snapshot() const
    {
        if (runtime_ == nullptr)
        {
            return AlarmOperatorSnapshotApiResult::failure(
                ApiStatus::InternalError,
                "runtime.alarm_operator_snapshot",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        return AlarmOperatorSnapshotApiResult::success(
            runtime_->alarm_operator_snapshot()
        );
    }

    dispatcher::runtime::DispatcherRuntime& DispatcherRuntimeApi::runtime()
        noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime&
        DispatcherRuntimeApi::runtime() const noexcept
    {
        return *runtime_;
    }
}