#pragma once

#include <dispatcher/api/history_api.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>

namespace dispatcher::api
{
    class DispatcherHistoryApi final : public HistoryApi
    {
    public:
        explicit DispatcherHistoryApi(
            dispatcher::runtime::DispatcherRuntime& runtime
        );

        [[nodiscard]] HistoryQueryApiResult query(
            const HistoryQueryRequest& request
        ) const override;

        [[nodiscard]] dispatcher::runtime::DispatcherRuntime& runtime()
            noexcept;

        [[nodiscard]] const dispatcher::runtime::DispatcherRuntime& runtime()
            const noexcept;

    private:
        dispatcher::runtime::DispatcherRuntime* runtime_{ nullptr };
    };
}