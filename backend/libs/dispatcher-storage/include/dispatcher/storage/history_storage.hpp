#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/history/history_sample.hpp>
#include <dispatcher/storage/history_sample_query_result.hpp>
#include <dispatcher/storage/history_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>

#include <vector>

namespace dispatcher::storage
{
    class HistoryStorage
    {
    public:
        virtual ~HistoryStorage() = default;

        [[nodiscard]] virtual StorageResult append(
            const dispatcher::history::HistorySample& sample
        ) = 0;

        [[nodiscard]] virtual StorageResult append_batch(
            const std::vector<dispatcher::history::HistorySample>& samples
        ) = 0;

        [[nodiscard]] virtual HistorySampleQueryResult query(
            const HistoryStorageQuery& query
        ) const = 0;

        [[nodiscard]] virtual StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) = 0;

        [[nodiscard]] virtual StorageResult clear() = 0;
    };
}