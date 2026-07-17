#pragma once

#include <dispatcher/api/api_page.hpp>
#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/history/history_sample.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::api
{
    class HistoryQueryApiResult
    {
    public:
        [[nodiscard]] static HistoryQueryApiResult success(
            std::vector<dispatcher::history::HistorySample> samples,
            ApiPage page
        );

        [[nodiscard]] static HistoryQueryApiResult failure(
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

        [[nodiscard]] const std::vector<dispatcher::history::HistorySample>&
            samples() const noexcept;

        [[nodiscard]] std::size_t sample_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] const ApiPage& page() const noexcept;

    private:
        HistoryQueryApiResult(
            ApiResult result,
            std::vector<dispatcher::history::HistorySample> samples,
            ApiPage page
        );

        ApiResult result_;
        std::vector<dispatcher::history::HistorySample> samples_;
        ApiPage page_;
    };
}