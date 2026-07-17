#include <dispatcher/config/configuration_import_model.hpp>

#include <utility>

namespace dispatcher::config
{
    ConfigurationImportModel::ConfigurationImportModel(
        ConfigurationImportMetadata metadata
    )
        : metadata_(std::move(metadata))
    {
    }

    ConfigurationImportModel ConfigurationImportModel::create_empty(
        ConfigurationFormat format
    )
    {
        ConfigurationImportMetadata metadata;

        metadata.format = format;

        return ConfigurationImportModel(std::move(metadata));
    }

    const ConfigurationImportMetadata& ConfigurationImportModel::metadata()
        const noexcept
    {
        return metadata_;
    }

    ConfigurationImportMetadata& ConfigurationImportModel::metadata()
        noexcept
    {
        return metadata_;
    }

    const std::vector<ConfigurationImportDevice>&
        ConfigurationImportModel::devices() const noexcept
    {
        return devices_;
    }

    const std::vector<ConfigurationImportTag>&
        ConfigurationImportModel::tags() const noexcept
    {
        return tags_;
    }

    void ConfigurationImportModel::add_device(
        ConfigurationImportDevice device
    )
    {
        devices_.push_back(std::move(device));
    }

    void ConfigurationImportModel::add_tag(ConfigurationImportTag tag)
    {
        tags_.push_back(std::move(tag));
    }

    void ConfigurationImportModel::clear_devices()
    {
        devices_.clear();
    }

    void ConfigurationImportModel::clear_tags()
    {
        tags_.clear();
    }

    void ConfigurationImportModel::clear()
    {
        clear_devices();
        clear_tags();
    }

    std::size_t ConfigurationImportModel::device_count() const noexcept
    {
        return devices_.size();
    }

    std::size_t ConfigurationImportModel::tag_count() const noexcept
    {
        return tags_.size();
    }

    bool ConfigurationImportModel::empty() const noexcept
    {
        return !has_devices() && !has_tags();
    }

    bool ConfigurationImportModel::has_devices() const noexcept
    {
        return !devices_.empty();
    }

    bool ConfigurationImportModel::has_tags() const noexcept
    {
        return !tags_.empty();
    }

    const ConfigurationImportDevice*
        ConfigurationImportModel::find_device_by_id(
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

    const ConfigurationImportTag* ConfigurationImportModel::find_tag_by_id(
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