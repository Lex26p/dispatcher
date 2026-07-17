#pragma once

#include <dispatcher/alarm/alarm_catalog.hpp>
#include <dispatcher/alarm/alarm_condition_catalog.hpp>
#include <dispatcher/alarm/alarm_configuration_metadata.hpp>
#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>
#include <dispatcher/domain/configuration_status.hpp>

#include <cstdint>

namespace dispatcher::alarm
{
    class AlarmConfigurationSnapshotBuilder
    {
    public:
        AlarmConfigurationSnapshotBuilder& config_version(std::uint64_t value);

        AlarmConfigurationSnapshotBuilder& metadata(
            AlarmConfigurationMetadata value
        );

        AlarmConfigurationSnapshotBuilder& status(
            dispatcher::domain::ConfigurationStatus value
        );

        AlarmConfigurationSnapshotBuilder& catalog(AlarmCatalog value);

        AlarmConfigurationSnapshotBuilder& alarm_catalog(AlarmCatalog value);

        AlarmConfigurationSnapshotBuilder& condition_catalog(
            AlarmConditionCatalog value
        );

        [[nodiscard]] AlarmConfigurationSnapshot build() const;

    private:
        std::uint64_t config_version_{ 1 };

        AlarmConfigurationMetadata metadata_{
            .name = "alarm-configuration",
            .description = "",
            .created_by = ""
        };

        dispatcher::domain::ConfigurationStatus status_{
            dispatcher::domain::ConfigurationStatus::Draft
        };

        AlarmCatalog alarm_catalog_;
        AlarmConditionCatalog condition_catalog_;
    };
}