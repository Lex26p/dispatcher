#include <dispatcher/config/configuration_export_metadata.hpp>

namespace dispatcher::config
{
    bool ConfigurationExportMetadata::has_schema_version() const noexcept
    {
        return !schema_version.empty();
    }

    bool ConfigurationExportMetadata::has_known_format() const noexcept
    {
        return is_known_configuration_format(format);
    }

    bool ConfigurationExportMetadata::has_config_version() const noexcept
    {
        return config_version > 0;
    }

    bool ConfigurationExportMetadata::has_status() const noexcept
    {
        return !status.empty();
    }

    bool ConfigurationExportMetadata::has_description() const noexcept
    {
        return !description.empty();
    }

    bool ConfigurationExportMetadata::has_source() const noexcept
    {
        return !source.empty();
    }

    bool ConfigurationExportMetadata::has_exported_at() const noexcept
    {
        return !exported_at.empty();
    }
}