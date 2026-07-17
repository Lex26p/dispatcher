#pragma once

#include <dispatcher/config/configuration_format.hpp>
#include <dispatcher/config/configuration_import_device.hpp>
#include <dispatcher/config/configuration_import_metadata.hpp>
#include <dispatcher/config/configuration_import_tag.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace dispatcher::config
{
    class ConfigurationImportModel
    {
    public:
        ConfigurationImportModel() = default;

        explicit ConfigurationImportModel(
            ConfigurationImportMetadata metadata
        );

        [[nodiscard]] static ConfigurationImportModel create_empty(
            ConfigurationFormat format = ConfigurationFormat::Json
        );

        [[nodiscard]] const ConfigurationImportMetadata& metadata()
            const noexcept;

        [[nodiscard]] ConfigurationImportMetadata& metadata() noexcept;

        [[nodiscard]] const std::vector<ConfigurationImportDevice>& devices()
            const noexcept;

        [[nodiscard]] const std::vector<ConfigurationImportTag>& tags()
            const noexcept;

        void add_device(ConfigurationImportDevice device);

        void add_tag(ConfigurationImportTag tag);

        void clear_devices();

        void clear_tags();

        void clear();

        [[nodiscard]] std::size_t device_count() const noexcept;

        [[nodiscard]] std::size_t tag_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_devices() const noexcept;

        [[nodiscard]] bool has_tags() const noexcept;

        [[nodiscard]] const ConfigurationImportDevice* find_device_by_id(
            std::string_view device_id
        ) const noexcept;

        [[nodiscard]] const ConfigurationImportTag* find_tag_by_id(
            std::string_view tag_id
        ) const noexcept;

    private:
        ConfigurationImportMetadata metadata_;
        std::vector<ConfigurationImportDevice> devices_;
        std::vector<ConfigurationImportTag> tags_;
    };
}