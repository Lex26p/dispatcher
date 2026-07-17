#include <dispatcher/alarm/alarm_configuration_snapshot_builder.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::config_version(std::uint64_t value)
    {
        config_version_ = value;
        return *this;
    }

    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::metadata(
            AlarmConfigurationMetadata value
        )
    {
        metadata_ = std::move(value);
        return *this;
    }

    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::status(
            dispatcher::domain::ConfigurationStatus value
        )
    {
        status_ = value;
        return *this;
    }

    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::catalog(AlarmCatalog value)
    {
        alarm_catalog_ = std::move(value);
        return *this;
    }

    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::alarm_catalog(AlarmCatalog value)
    {
        alarm_catalog_ = std::move(value);
        return *this;
    }

    AlarmConfigurationSnapshotBuilder&
        AlarmConfigurationSnapshotBuilder::condition_catalog(
            AlarmConditionCatalog value
        )
    {
        condition_catalog_ = std::move(value);
        return *this;
    }

    AlarmConfigurationSnapshot AlarmConfigurationSnapshotBuilder::build() const
    {
        return AlarmConfigurationSnapshot(
            config_version_,
            metadata_,
            status_,
            alarm_catalog_,
            condition_catalog_
        );
    }
}