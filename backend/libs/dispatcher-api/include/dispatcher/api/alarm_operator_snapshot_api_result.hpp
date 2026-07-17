#pragma once

#include <dispatcher/alarm/alarm_operator_snapshot.hpp>
#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class AlarmOperatorSnapshotApiResult
    {
    public:
        [[nodiscard]] static AlarmOperatorSnapshotApiResult success(
            dispatcher::alarm::AlarmOperatorSnapshot snapshot
        );

        [[nodiscard]] static AlarmOperatorSnapshotApiResult failure(
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

        [[nodiscard]] const dispatcher::alarm::AlarmOperatorSnapshot&
            snapshot() const;

    private:
        AlarmOperatorSnapshotApiResult(
            ApiResult result,
            std::optional<dispatcher::alarm::AlarmOperatorSnapshot> snapshot
        );

        ApiResult result_;
        std::optional<dispatcher::alarm::AlarmOperatorSnapshot> snapshot_;
    };
}