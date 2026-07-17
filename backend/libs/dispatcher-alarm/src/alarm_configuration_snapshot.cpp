#include <dispatcher/alarm/alarm_configuration_snapshot.hpp>

#include <utility>

namespace dispatcher::alarm
{
    AlarmConfigurationSnapshot::AlarmConfigurationSnapshot(
        std::uint64_t config_version,
        AlarmConfigurationMetadata metadata,
        dispatcher::domain::ConfigurationStatus status,
        AlarmCatalog alarm_catalog,
        AlarmConditionCatalog condition_catalog
    )
        : config_version_(config_version)
        , metadata_(std::move(metadata))
        , status_(status)
        , alarm_catalog_(std::move(alarm_catalog))
        , condition_catalog_(std::move(condition_catalog))
    {
    }

    std::uint64_t AlarmConfigurationSnapshot::config_version() const noexcept
    {
        return config_version_;
    }

    const AlarmConfigurationMetadata& AlarmConfigurationSnapshot::metadata()
        const noexcept
    {
        return metadata_;
    }

    dispatcher::domain::ConfigurationStatus AlarmConfigurationSnapshot::status()
        const noexcept
    {
        return status_;
    }

    bool AlarmConfigurationSnapshot::draft() const noexcept
    {
        return status_ == dispatcher::domain::ConfigurationStatus::Draft;
    }

    bool AlarmConfigurationSnapshot::published() const noexcept
    {
        return status_ == dispatcher::domain::ConfigurationStatus::Published;
    }

    const AlarmCatalog& AlarmConfigurationSnapshot::alarm_catalog() const noexcept
    {
        return alarm_catalog_;
    }

    const AlarmCatalog& AlarmConfigurationSnapshot::catalog() const noexcept
    {
        return alarm_catalog_;
    }

    const AlarmConditionCatalog& AlarmConfigurationSnapshot::condition_catalog()
        const noexcept
    {
        return condition_catalog_;
    }

    std::optional<AlarmDefinition> AlarmConfigurationSnapshot::find_by_alarm_id(
        const dispatcher::domain::AlarmId& alarm_id
    ) const
    {
        return alarm_catalog_.find_by_alarm_id(alarm_id);
    }

    std::vector<AlarmDefinition> AlarmConfigurationSnapshot::find_by_tag_id(
        const dispatcher::domain::TagId& tag_id
    ) const
    {
        return alarm_catalog_.find_by_tag_id(tag_id);
    }
}