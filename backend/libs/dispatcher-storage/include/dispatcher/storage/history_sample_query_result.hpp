#pragma once

#include <dispatcher/history/history_sample.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::storage
{
    class HistorySampleQueryResult
    {
    public:
        [[nodiscard]] static HistorySampleQueryResult success(
            std::vector<dispatcher::history::HistorySample> samples
        );

        [[nodiscard]] static HistorySampleQueryResult failure(
            StorageStatus status,
            std::string operation = {},
            std::string key = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] StorageStatus status() const noexcept;

        [[nodiscard]] const StorageResult& result() const noexcept;

        [[nodiscard]] const StorageError& error() const noexcept;

        [[nodiscard]] const std::vector<dispatcher::history::HistorySample>&
            samples() const noexcept;

        [[nodiscard]] std::size_t sample_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        HistorySampleQueryResult(
            StorageResult result,
            std::vector<dispatcher::history::HistorySample> samples
        );

        StorageResult result_;
        std::vector<dispatcher::history::HistorySample> samples_;
    };
}