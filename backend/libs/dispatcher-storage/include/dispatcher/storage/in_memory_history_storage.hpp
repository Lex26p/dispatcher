#pragma once

#include <dispatcher/storage/history_storage.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <cstddef>
#include <vector>

namespace dispatcher::storage
{
    class InMemoryHistoryStorage final : public HistoryStorage
    {
    public:
        [[nodiscard]] StorageResult append(
            const dispatcher::history::HistorySample& sample
        ) override
        {
            samples_.push_back(sample);

            return StorageResult::success();
        }

        [[nodiscard]] StorageResult append_batch(
            const std::vector<dispatcher::history::HistorySample>& samples
        ) override
        {
            samples_.insert(
                samples_.end(),
                samples.begin(),
                samples.end()
            );

            return StorageResult::success();
        }

        [[nodiscard]] HistorySampleQueryResult query(
            const HistoryStorageQuery& query
        ) const override
        {
            auto selected_samples = samples_;

            if (query.requests_latest_only())
            {
                if (selected_samples.empty())
                {
                    return HistorySampleQueryResult::success({});
                }

                return HistorySampleQueryResult::success(
                    std::vector<dispatcher::history::HistorySample>{
                    selected_samples.back()
                }
                );
            }

            if (
                query.has_limit()
                && selected_samples.size() > query.limit
                )
            {
                selected_samples.erase(
                    selected_samples.begin(),
                    selected_samples.end()
                    - static_cast<std::vector<
                    dispatcher::history::HistorySample
                    >::difference_type>(query.limit)
                );
            }

            return HistorySampleQueryResult::success(
                std::move(selected_samples)
            );
        }

        [[nodiscard]] StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) override
        {
            return StorageResult::failure(
                StorageStatus::UnsupportedOperation,
                "history.remove_by_tag",
                tag_id.value(),
                "remove_by_tag is not implemented by baseline in-memory history storage"
            );
        }

        [[nodiscard]] StorageResult clear() override
        {
            samples_.clear();

            return StorageResult::success();
        }

        [[nodiscard]] const std::vector<dispatcher::history::HistorySample>&
            samples() const noexcept
        {
            return samples_;
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return samples_.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return samples_.empty();
        }

    private:
        std::vector<dispatcher::history::HistorySample> samples_;
    };
}