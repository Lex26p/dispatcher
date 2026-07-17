#include <dispatcher/domain/configuration_snapshot_builder.hpp>

#include <dispatcher/domain/configuration_snapshot_validation.hpp>

#include <utility>

namespace dispatcher::domain
{
    ConfigurationSnapshotBuilder& ConfigurationSnapshotBuilder::config_version(
        std::uint64_t value
    )
    {
        config_version_ = value;
        return *this;
    }

    ConfigurationSnapshotBuilder& ConfigurationSnapshotBuilder::status(
        ConfigurationStatus value
    )
    {
        status_ = value;
        return *this;
    }

    ConfigurationSnapshotBuilder& ConfigurationSnapshotBuilder::description(
        std::string value
    )
    {
        description_ = std::move(value);
        return *this;
    }

    ConfigurationSnapshotBuilder& ConfigurationSnapshotBuilder::draft()
    {
        status_ = ConfigurationStatus::Draft;
        return *this;
    }

    ConfigurationSnapshotBuilder& ConfigurationSnapshotBuilder::published()
    {
        status_ = ConfigurationStatus::Published;
        return *this;
    }

    dispatcher::common::ValidationResult ConfigurationSnapshotBuilder::add_device(
        DeviceDefinition device_definition
    )
    {
        return device_catalog_.add(std::move(device_definition));
    }

    dispatcher::common::ValidationResult ConfigurationSnapshotBuilder::add_tag(
        TagDefinition tag_definition
    )
    {
        return tag_catalog_.add(std::move(tag_definition));
    }

    ConfigurationSnapshot ConfigurationSnapshotBuilder::build() const
    {
        return ConfigurationSnapshot(
            ConfigurationMetadata(
                config_version_,
                status_,
                description_,
                ConfigurationMetadata::Clock::now()
            ),
            device_catalog_,
            tag_catalog_
        );
    }

    dispatcher::common::ValidationResult ConfigurationSnapshotBuilder::validate() const
    {
        return validate_configuration_snapshot(build());
    }

    const DeviceCatalog& ConfigurationSnapshotBuilder::device_catalog() const noexcept
    {
        return device_catalog_;
    }

    const TagCatalog& ConfigurationSnapshotBuilder::tag_catalog() const noexcept
    {
        return tag_catalog_;
    }
}