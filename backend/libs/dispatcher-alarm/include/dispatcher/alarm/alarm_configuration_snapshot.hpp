#pragma once

#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_definition.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmConfigurationSnapshot
    {
    public:
        AlarmConfigurationSnapshot(
            std::uint64_t config_version,
            AlarmConfigurationMetadata metadata,
            dispatcher::domain::ConfigurationStatus status,
            AlarmCatalog alarm_catalog,
            AlarmConditionCatalog condition_catalog
        );

        [[nodiscard]] std::uint64_t config_version() const noexcept;

        [[nodiscard]] const AlarmConfigurationMetadata& metadata() const noexcept;

        [[nodiscard]] dispatcher::domain::ConfigurationStatus status() const noexcept;

        [[nodiscard]] bool draft() const noexcept;

        [[nodiscard]] bool published() const noexcept;

        [[nodiscard]] const AlarmCatalog& alarm_catalog() const noexcept;

        [[nodiscard]] const AlarmCatalog& catalog() const noexcept;

        [[nodiscard]] const AlarmConditionCatalog& condition_catalog() const noexcept;

        [[nodiscard]] std::optional<AlarmDefinition> find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] std::vector<AlarmDefinition> find_by_tag_id(
            const dispatcher::domain::TagId& tag_id
        ) const;

    private:
        std::uint64_t config_version_;
        AlarmConfigurationMetadata metadata_;
        dispatcher::domain::ConfigurationStatus status_;
        AlarmCatalog alarm_catalog_;
        AlarmConditionCatalog condition_catalog_;
    };
}