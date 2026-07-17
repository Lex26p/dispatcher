#include <dispatcher/config/configuration_export_model.hpp>

#include <utility>

namespace dispatcher::config
{
    ConfigurationExportModel::ConfigurationExportModel(
        ConfigurationExportMetadata metadata
    )
        : metadata_(std::move(metadata))
    {
    }

    ConfigurationExportModel ConfigurationExportModel::create_empty(
        ConfigurationFormat format
    )
    {
        ConfigurationExportMetadata metadata;

        metadata.format = format;

        return ConfigurationExportModel(std::move(metadata));
    }

    const ConfigurationExportMetadata& ConfigurationExportModel::metadata()
        const noexcept
    {
        return metadata_;
    }

    ConfigurationExportMetadata& ConfigurationExportModel::metadata()
        noexcept
    {
        return metadata_;
    }

    const std::vector<ConfigurationExportDevice>&
        ConfigurationExportModel::devices() const noexcept
    {
        return devices_;
    }

    const std::vector<ConfigurationExportTag>&
        ConfigurationExportModel::tags() const noexcept
    {
        return tags_;
    }

    void ConfigurationExportModel::add_device(
        ConfigurationExportDevice device
    )
    {
        devices_.push_back(std::move(device));
    }

    void ConfigurationExportModel::add_tag(ConfigurationExportTag tag)
    {
        tags_.push_back(std::move(tag));
    }

    void ConfigurationExportModel::clear_devices()
    {
        devices_.clear();
    }

    void ConfigurationExportModel::clear_tags()
    {
        tags_.clear();
    }

    void ConfigurationExportModel::clear()
    {
        clear_devices();
        clear_tags();
    }

    std::size_t ConfigurationExportModel::device_count() const noexcept
    {
        return devices_.size();
    }

    std::size_t ConfigurationExportModel::tag_count() const noexcept
    {
        return tags_.size();
    }

    bool ConfigurationExportModel::empty() const noexcept
    {
        return !has_devices() && !has_tags();
    }

    bool ConfigurationExportModel::has_devices() const noexcept
    {
        return !devices_.empty();
    }

    bool ConfigurationExportModel::has_tags() const noexcept
    {
        return !tags_.empty();
    }

    const ConfigurationExportDevice*
        ConfigurationExportModel::find_device_by_id(
            std::string_view device_id
        ) const noexcept
    {
        for (const auto& device : devices_)
        {
            if (device.device_id == device_id)
            {
                return &device;
            }
        }

        return nullptr;
    }

    const ConfigurationExportTag* ConfigurationExportModel::find_tag_by_id(
        std::string_view tag_id
    ) const noexcept
    {
        for (const auto& tag : tags_)
        {
            if (tag.tag_id == tag_id)
            {
                return &tag;
            }
        }

        return nullptr;
    }
}