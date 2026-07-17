#include <dispatcher/config/configuration_import_metadata.hpp>

namespace dispatcher::config
{
    bool ConfigurationImportMetadata::has_schema_version() const noexcept
    {
        return !schema_version.empty();
    }

    bool ConfigurationImportMetadata::has_known_format() const noexcept
    {
        return is_known_configuration_format(format);
    }

    bool ConfigurationImportMetadata::has_config_version() const noexcept
    {
        return config_version > 0;
    }

    bool ConfigurationImportMetadata::has_status() const noexcept
    {
        return !status.empty();
    }

    bool ConfigurationImportMetadata::has_description() const noexcept
    {
        return !description.empty();
    }

    bool ConfigurationImportMetadata::has_source() const noexcept
    {
        return !source.empty();
    }

    bool ConfigurationImportMetadata::has_imported_at() const noexcept
    {
        return !imported_at.empty();
    }
}