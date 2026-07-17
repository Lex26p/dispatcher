#pragma once

#include <dispatcher/storage/in_memory_alarm_acknowledgement_storage.hpp>
#include <dispatcher/storage/in_memory_alarm_event_storage.hpp>
#include <dispatcher/storage/in_memory_configuration_storage.hpp>
#include <dispatcher/storage/in_memory_history_storage.hpp>
#include <dispatcher/storage/storage_repository.hpp>

namespace dispatcher::storage
{
    class InMemoryStorageRepository final : public StorageRepository
    {
    public:
        [[nodiscard]] HistoryStorage& history_storage() noexcept override
        {
            return history_storage_;
        }

        [[nodiscard]] const HistoryStorage& history_storage()
            const noexcept override
        {
            return history_storage_;
        }

        [[nodiscard]] AlarmEventStorage& alarm_event_storage()
            noexcept override
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] const AlarmEventStorage& alarm_event_storage()
            const noexcept override
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() noexcept override
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] const AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() const noexcept override
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] ConfigurationStorage& configuration_storage()
            noexcept override
        {
            return configuration_storage_;
        }

        [[nodiscard]] const ConfigurationStorage& configuration_storage()
            const noexcept override
        {
            return configuration_storage_;
        }

        [[nodiscard]] InMemoryHistoryStorage& in_memory_history_storage()
            noexcept
        {
            return history_storage_;
        }

        [[nodiscard]] const InMemoryHistoryStorage&
            in_memory_history_storage() const noexcept
        {
            return history_storage_;
        }

        [[nodiscard]] InMemoryAlarmEventStorage&
            in_memory_alarm_event_storage() noexcept
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] const InMemoryAlarmEventStorage&
            in_memory_alarm_event_storage() const noexcept
        {
            return alarm_event_storage_;
        }

        [[nodiscard]] InMemoryAlarmAcknowledgementStorage&
            in_memory_alarm_acknowledgement_storage() noexcept
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] const InMemoryAlarmAcknowledgementStorage&
            in_memory_alarm_acknowledgement_storage() const noexcept
        {
            return alarm_acknowledgement_storage_;
        }

        [[nodiscard]] InMemoryConfigurationStorage&
            in_memory_configuration_storage() noexcept
        {
            return configuration_storage_;
        }

        [[nodiscard]] const InMemoryConfigurationStorage&
            in_memory_configuration_storage() const noexcept
        {
            return configuration_storage_;
        }

    private:
        InMemoryHistoryStorage history_storage_;
        InMemoryAlarmEventStorage alarm_event_storage_;
        InMemoryAlarmAcknowledgementStorage alarm_acknowledgement_storage_;
        InMemoryConfigurationStorage configuration_storage_;
    };
}