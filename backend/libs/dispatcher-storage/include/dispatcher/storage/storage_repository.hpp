#pragma once

#include <dispatcher/storage/alarm_acknowledgement_storage.hpp>
#include <dispatcher/storage/alarm_event_storage.hpp>
#include <dispatcher/storage/configuration_storage.hpp>
#include <dispatcher/storage/history_storage.hpp>

namespace dispatcher::storage
{
    class StorageRepository
    {
    public:
        virtual ~StorageRepository() = default;

        [[nodiscard]] virtual HistoryStorage& history_storage() noexcept = 0;

        [[nodiscard]] virtual const HistoryStorage& history_storage()
            const noexcept = 0;

        [[nodiscard]] virtual AlarmEventStorage& alarm_event_storage()
            noexcept = 0;

        [[nodiscard]] virtual const AlarmEventStorage& alarm_event_storage()
            const noexcept = 0;

        [[nodiscard]] virtual AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() noexcept = 0;

        [[nodiscard]] virtual const AlarmAcknowledgementStorage&
            alarm_acknowledgement_storage() const noexcept = 0;

        [[nodiscard]] virtual ConfigurationStorage& configuration_storage()
            noexcept = 0;

        [[nodiscard]] virtual const ConfigurationStorage&
            configuration_storage() const noexcept = 0;
    };
}