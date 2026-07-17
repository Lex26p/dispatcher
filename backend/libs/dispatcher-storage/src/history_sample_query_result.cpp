#include <dispatcher/storage/history_sample_query_result.hpp>

#include <utility>

namespace dispatcher::storage
{
    HistorySampleQueryResult HistorySampleQueryResult::success(
        std::vector<dispatcher::history::HistorySample> samples
    )
    {
        return HistorySampleQueryResult(
            StorageResult::success(),
            std::move(samples)
        );
    }

    HistorySampleQueryResult HistorySampleQueryResult::failure(
        StorageStatus status,
        std::string operation,
        std::string key,
        std::string message
    )
    {
        return HistorySampleQueryResult(
            StorageResult::failure(
                status,
                std::move(operation),
                std::move(key),
                std::move(message)
            ),
            {}
        );
    }

    bool HistorySampleQueryResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool HistorySampleQueryResult::failed() const noexcept
    {
        return result_.failed();
    }

    StorageStatus HistorySampleQueryResult::status() const noexcept
    {
        return result_.status();
    }

    const StorageResult& HistorySampleQueryResult::result() const noexcept
    {
        return result_;
    }

    const StorageError& HistorySampleQueryResult::error() const noexcept
    {
        return result_.error();
    }

    const std::vector<dispatcher::history::HistorySample>&
        HistorySampleQueryResult::samples() const noexcept
    {
        return samples_;
    }

    std::size_t HistorySampleQueryResult::sample_count() const noexcept
    {
        return samples_.size();
    }

    bool HistorySampleQueryResult::empty() const noexcept
    {
        return samples_.empty();
    }

    HistorySampleQueryResult::HistorySampleQueryResult(
        StorageResult result,
        std::vector<dispatcher::history::HistorySample> samples
    )
        : result_(std::move(result))
        , samples_(std::move(samples))
    {
    }
}