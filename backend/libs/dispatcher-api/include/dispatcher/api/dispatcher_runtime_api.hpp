#pragma once

#include <dispatcher/api/runtime_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

namespace dispatcher::api
{
    class DispatcherRuntimeApi final : public RuntimeApi
    {
    public:
        explicit DispatcherRuntimeApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] RuntimeSnapshotApiResult runtime_snapshot()
            const override;

        [[nodiscard]] AlarmOperatorSnapshotApiResult
            alarm_operator_snapshot() const override;

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };
    };
}