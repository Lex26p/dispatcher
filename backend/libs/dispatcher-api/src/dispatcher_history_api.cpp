#include <dispatcher/api/dispatcher_history_api.hpp>

#include <dispatcher/api/api_page.hpp>
#include <dispatcher/api/api_status_mapping.hpp>

namespace dispatcher::api
{
    DispatcherHistoryApi::DispatcherHistoryApi(
        dispatcher::runtime::DispatcherRuntime& runtime
    )
        : runtime_(&runtime)
    {
    }

    HistoryQueryApiResult DispatcherHistoryApi::query(
        const HistoryQueryRequest& request
    ) const
    {
        if (runtime_ == nullptr)
        {
            return HistoryQueryApiResult::failure(
                ApiStatus::InternalError,
                "history.query",
                "runtime",
                {},
                "runtime instance is not available"
            );
        }

        auto* storage_repository = runtime_->storage_repository();

        if (storage_repository == nullptr)
        {
            return HistoryQueryApiResult::failure(
                ApiStatus::StorageError,
                "history.query",
                "storage_repository",
                {},
                "runtime does not have a storage repository"
            );
        }

        const auto storage_query_result =
            storage_repository->history_storage().query(
                request.to_storage_query()
            );

        if (storage_query_result.failed())
        {
            return HistoryQueryApiResult::failure(
                map_storage_status_to_api_status(
                    storage_query_result.status()
                ),
                storage_query_result.error().has_operation()
                ? storage_query_result.error().operation
                : "history.query",
                storage_query_result.error().key,
                {},
                storage_query_result.error().message
            );
        }

        auto samples = storage_query_result.samples();

        const auto page = ApiPage::from_request(
            request.page,
            samples.size(),
            samples.size()
        );

        return HistoryQueryApiResult::success(
            std::move(samples),
            page
        );
    }

    dispatcher::runtime::DispatcherRuntime&
        DispatcherHistoryApi::runtime() noexcept
    {
        return *runtime_;
    }

    const dispatcher::runtime::DispatcherRuntime&
        DispatcherHistoryApi::runtime() const noexcept
    {
        return *runtime_;
    }
}