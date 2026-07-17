#include <dispatcher/alarm/alarm_suppression_store.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::alarm
{
    AlarmSuppressionResult AlarmSuppressionStore::apply(
        const AlarmSuppressionCommand& command,
        TimePoint now
    )
    {
        if (
            !command.has_alarm_id()
            || !command.has_operator_id()
            || !is_known_reason(command.reason())
            )
        {
            ++rejected_count_;

            return AlarmSuppressionResult::failure(
                AlarmSuppressionStatus::InvalidCommand,
                "alarm suppression command is invalid"
            );
        }

        if (command.expired_at(now))
        {
            ++rejected_count_;

            return AlarmSuppressionResult::failure(
                AlarmSuppressionStatus::Expired,
                "alarm suppression command is expired"
            );
        }

        auto existing = find_iterator(command.alarm_id());

        if (existing != records_.end())
        {
            if (existing->expired_at(now))
            {
                records_.erase(existing);

                ++expired_removed_count_;
            }
            else
            {
                ++rejected_count_;

                return AlarmSuppressionResult::failure(
                    AlarmSuppressionStatus::AlreadySuppressed,
                    "alarm is already suppressed"
                );
            }
        }

        records_.push_back(
            AlarmSuppressionRecord::from_command(command, now)
        );

        ++applied_count_;

        return AlarmSuppressionResult::applied(records_.back());
    }

    AlarmSuppressionResult AlarmSuppressionStore::clear(
        const dispatcher::domain::AlarmId& alarm_id,
        TimePoint now
    )
    {
        if (alarm_id.value().empty())
        {
            ++rejected_count_;

            return AlarmSuppressionResult::failure(
                AlarmSuppressionStatus::InvalidCommand,
                "alarm id is required"
            );
        }

        auto existing = find_iterator(alarm_id);

        if (existing == records_.end())
        {
            ++rejected_count_;

            return AlarmSuppressionResult::failure(
                AlarmSuppressionStatus::NotSuppressed,
                "alarm is not suppressed"
            );
        }

        if (existing->expired_at(now))
        {
            records_.erase(existing);

            ++rejected_count_;
            ++expired_removed_count_;

            return AlarmSuppressionResult::failure(
                AlarmSuppressionStatus::Expired,
                "alarm suppression has expired"
            );
        }

        auto record = *existing;

        records_.erase(existing);

        ++cleared_count_;

        return AlarmSuppressionResult::cleared(std::move(record));
    }

    const AlarmSuppressionRecord* AlarmSuppressionStore::find_by_alarm_id(
        const dispatcher::domain::AlarmId& alarm_id,
        TimePoint now
    ) const noexcept
    {
        const auto existing = find_iterator(alarm_id);

        if (existing == records_.end())
        {
            return nullptr;
        }

        if (existing->expired_at(now))
        {
            return nullptr;
        }

        return &(*existing);
    }

    bool AlarmSuppressionStore::is_active(
        const dispatcher::domain::AlarmId& alarm_id,
        TimePoint now
    ) const noexcept
    {
        return find_by_alarm_id(alarm_id, now) != nullptr;
    }

    std::vector<AlarmSuppressionRecord> AlarmSuppressionStore::active_records(
        TimePoint now
    ) const
    {
        std::vector<AlarmSuppressionRecord> active;

        for (const auto& record : records_)
        {
            if (record.active_at(now))
            {
                active.push_back(record);
            }
        }

        return active;
    }

    const std::vector<AlarmSuppressionRecord>&
        AlarmSuppressionStore::records() const noexcept
    {
        return records_;
    }

    std::size_t AlarmSuppressionStore::size() const noexcept
    {
        return records_.size();
    }

    bool AlarmSuppressionStore::empty() const noexcept
    {
        return records_.empty();
    }

    std::size_t AlarmSuppressionStore::remove_expired(TimePoint now)
    {
        const auto before = records_.size();

        records_.erase(
            std::remove_if(
                records_.begin(),
                records_.end(),
                [now](const AlarmSuppressionRecord& record)
                {
                    return record.expired_at(now);
                }
            ),
            records_.end()
        );

        const auto removed = before - records_.size();

        expired_removed_count_ += removed;

        return removed;
    }

    void AlarmSuppressionStore::clear_all()
    {
        records_.clear();
    }

    AlarmSuppressionRuntimeSnapshot AlarmSuppressionStore::runtime_snapshot(
        TimePoint now
    ) const noexcept
    {
        AlarmSuppressionRuntimeSnapshot snapshot;

        snapshot.store_size = records_.size();

        for (const auto& record : records_)
        {
            if (record.expired_at(now))
            {
                ++snapshot.expired_count;
                continue;
            }

            ++snapshot.active_count;

            if (record.mode() == AlarmSuppressionMode::Shelved)
            {
                ++snapshot.shelved_count;
            }
            else if (record.mode() == AlarmSuppressionMode::Suppressed)
            {
                ++snapshot.suppressed_count;
            }
            else if (record.mode() == AlarmSuppressionMode::Inhibited)
            {
                ++snapshot.inhibited_count;
            }

            if (is_operator_controlled(record.mode()))
            {
                ++snapshot.operator_controlled_count;
            }

            if (is_system_controlled(record.mode()))
            {
                ++snapshot.system_controlled_count;
            }
        }

        snapshot.applied_count = applied_count_;
        snapshot.cleared_count = cleared_count_;
        snapshot.rejected_count = rejected_count_;
        snapshot.expired_removed_count = expired_removed_count_;

        return snapshot;
    }

    void AlarmSuppressionStore::reset_statistics()
    {
        applied_count_ = 0;
        cleared_count_ = 0;
        rejected_count_ = 0;
        expired_removed_count_ = 0;
    }

    AlarmSuppressionStore::RecordIterator
        AlarmSuppressionStore::find_iterator(
            const dispatcher::domain::AlarmId& alarm_id
        ) noexcept
    {
        return std::find_if(
            records_.begin(),
            records_.end(),
            [&alarm_id](const AlarmSuppressionRecord& record)
            {
                return record.alarm_id() == alarm_id;
            }
        );
    }

    AlarmSuppressionStore::ConstRecordIterator
        AlarmSuppressionStore::find_iterator(
            const dispatcher::domain::AlarmId& alarm_id
        ) const noexcept
    {
        return std::find_if(
            records_.begin(),
            records_.end(),
            [&alarm_id](const AlarmSuppressionRecord& record)
            {
                return record.alarm_id() == alarm_id;
            }
        );
    }
}