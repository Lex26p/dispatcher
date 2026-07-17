#pragma once

#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime_snapshot.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class RuntimeSnapshotApiResult
    {
    public:
        [[nodiscard]] static RuntimeSnapshotApiResult success(
            dispatcher::runtime::DispatcherRuntimeSnapshot snapshot
        );

        [[nodiscard]] static RuntimeSnapshotApiResult failure(
            ApiStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ApiStatus status() const noexcept;

        [[nodiscard]] const ApiResult& result() const noexcept;

        [[nodiscard]] const ApiError& error() const noexcept;

        [[nodiscard]] bool has_snapshot() const noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntimeSnapshot&
            snapshot() const;

    private:
        RuntimeSnapshotApiResult(
            ApiResult result,
            std::optional<dispatcher::runtime::DispatcherRuntimeSnapshot>
            snapshot
        );

        ApiResult result_;
        std::optional<dispatcher::runtime::DispatcherRuntimeSnapshot> snapshot_;
    };
}