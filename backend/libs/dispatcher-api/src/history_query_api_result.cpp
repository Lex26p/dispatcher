#include <dispatcher/api/history_query_api_result.hpp>

#include <utility>

namespace dispatcher::api
{
    HistoryQueryApiResult HistoryQueryApiResult::success(
        std::vector<dispatcher::history::HistorySample> samples,
        ApiPage page
    )
    {
        return HistoryQueryApiResult(
            ApiResult::success(),
            std::move(samples),
            page
        );
    }

    HistoryQueryApiResult HistoryQueryApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return HistoryQueryApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            {},
            ApiPage{}
        );
    }

    bool HistoryQueryApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool HistoryQueryApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus HistoryQueryApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& HistoryQueryApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& HistoryQueryApiResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::history::HistorySample>&
        HistoryQueryApiResult::samples() const noexcept
    {
        return samples_;
    }

    std::size_t HistoryQueryApiResult::sample_count() const noexcept
    {
        return samples_.size();
    }

    bool HistoryQueryApiResult::empty() const noexcept
    {
        return samples_.empty();
    }

    const ApiPage& HistoryQueryApiResult::page() const noexcept
    {
        return page_;
    }

    HistoryQueryApiResult::HistoryQueryApiResult(
        ApiResult result,
        std::vector<dispatcher::history::HistorySample> samples,
        ApiPage page
    )
        : result_(std::move(result))
        , samples_(std::move(samples))
        , page_(page)
    {
    }
}