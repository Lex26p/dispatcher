#pragma once

#include <dispatcher/config/configuration_export_device.hpp>
#include <dispatcher/config/configuration_export_metadata.hpp>
#include <dispatcher/config/configuration_export_tag.hpp>
#include <dispatcher/config/configuration_format.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace dispatcher::config
{
    class ConfigurationExportModel
    {
    public:
        ConfigurationExportModel() = default;

        explicit ConfigurationExportModel(
            ConfigurationExportMetadata metadata
        );

        [[nodiscard]] static ConfigurationExportModel create_empty(
            ConfigurationFormat format = ConfigurationFormat::Json
        );

        [[nodiscard]] const ConfigurationExportMetadata& metadata()
            const noexcept;

        [[nodiscard]] ConfigurationExportMetadata& metadata() noexcept;

        [[nodiscard]] const std::vector<ConfigurationExportDevice>& devices()
            const noexcept;

        [[nodiscard]] const std::vector<ConfigurationExportTag>& tags()
            const noexcept;

        void add_device(ConfigurationExportDevice device);

        void add_tag(ConfigurationExportTag tag);

        void clear_devices();

        void clear_tags();

        void clear();

        [[nodiscard]] std::size_t device_count() const noexcept;

        [[nodiscard]] std::size_t tag_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_devices() const noexcept;

        [[nodiscard]] bool has_tags() const noexcept;

        [[nodiscard]] const ConfigurationExportDevice* find_device_by_id(
            std::string_view device_id
        ) const noexcept;

        [[nodiscard]] const ConfigurationExportTag* find_tag_by_id(
            std::string_view tag_id
        ) const noexcept;

    private:
        ConfigurationExportMetadata metadata_;
        std::vector<ConfigurationExportDevice> devices_;
        std::vector<ConfigurationExportTag> tags_;
    };
}