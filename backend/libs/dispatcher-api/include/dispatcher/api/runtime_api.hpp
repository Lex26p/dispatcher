#pragma once

#include <dispatcher/api/alarm_operator_snapshot_api_result.hpp>
#include <dispatcher/api/runtime_snapshot_api_result.hpp>

namespace dispatcher::api
{
    class RuntimeApi
    {
    public:
        virtual ~RuntimeApi() = default;

        [[nodiscard]] virtual RuntimeSnapshotApiResult runtime_snapshot()
            const = 0;

        [[nodiscard]] virtual AlarmOperatorSnapshotApiResult
            alarm_operator_snapshot() const = 0;
    };
}