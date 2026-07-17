#pragma once

#include <dispatcher/api/history_query_api_result.hpp>
#include <dispatcher/api/history_query_request.hpp>

namespace dispatcher::api
{
    class HistoryApi
    {
    public:
        virtual ~HistoryApi() = default;

        [[nodiscard]] virtual HistoryQueryApiResult query(
            const HistoryQueryRequest& request
        ) const = 0;
    };
}