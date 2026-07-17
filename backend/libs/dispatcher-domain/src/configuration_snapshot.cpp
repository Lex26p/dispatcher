#include <dispatcher/domain/configuration_snapshot.hpp>

#include <utility>

namespace dispatcher::domain
{
    ConfigurationSnapshot::ConfigurationSnapshot(
        std::uint64_t config_version,
        DeviceCatalog device_catalog,
        TagCatalog tag_catalog
    )
        : ConfigurationSnapshot(
            ConfigurationMetadata::published(config_version),
            std::move(device_catalog),
            std::move(tag_catalog)
        )
    {
    }

    ConfigurationSnapshot::ConfigurationSnapshot(
        ConfigurationMetadata metadata,
        DeviceCatalog device_catalog,
        TagCatalog tag_catalog
    )
        : metadata_(std::move(metadata))
        , device_catalog_(std::move(device_catalog))
        , tag_catalog_(std::move(tag_catalog))
    {
    }

    const ConfigurationMetadata& ConfigurationSnapshot::metadata() const noexcept
    {
        return metadata_;
    }

    std::uint64_t ConfigurationSnapshot::config_version() const noexcept
    {
        return metadata_.config_version();
    }

    ConfigurationStatus ConfigurationSnapshot::status() const noexcept
    {
        return metadata_.status();
    }

    bool ConfigurationSnapshot::is_draft() const noexcept
    {
        return metadata_.is_draft();
    }

    bool ConfigurationSnapshot::is_published() const noexcept
    {
        return metadata_.is_published();
    }

    const DeviceCatalog& ConfigurationSnapshot::device_catalog() const noexcept
    {
        return device_catalog_;
    }

    const TagCatalog& ConfigurationSnapshot::tag_catalog() const noexcept
    {
        return tag_catalog_;
    }

    std::size_t ConfigurationSnapshot::device_count() const noexcept
    {
        return device_catalog_.size();
    }

    std::size_t ConfigurationSnapshot::tag_count() const noexcept
    {
        return tag_catalog_.size();
    }

    const std::vector<DeviceDefinition>& ConfigurationSnapshot::devices()
        const noexcept
    {
        return device_catalog_.devices();
    }

    const std::vector<TagDefinition>& ConfigurationSnapshot::tags()
        const noexcept
    {
        return tag_catalog_.tags();
    }

    bool ConfigurationSnapshot::empty() const noexcept
    {
        return device_catalog_.empty() && tag_catalog_.empty();
    }

    std::optional<DeviceDefinition> ConfigurationSnapshot::find_device_by_id(
        const DeviceId& device_id
    ) const
    {
        return device_catalog_.find_by_id(device_id);
    }

    std::optional<TagDefinition> ConfigurationSnapshot::find_tag_by_id(
        const TagId& tag_id
    ) const
    {
        return tag_catalog_.find_by_id(tag_id);
    }
}