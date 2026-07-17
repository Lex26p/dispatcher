#include <dispatcher/config/configuration_export_options.hpp>

namespace dispatcher::config
{
    bool ConfigurationExportOptions::has_known_format() const noexcept
    {
        return is_known_configuration_format(format);
    }

    bool ConfigurationExportOptions::has_source() const noexcept
    {
        return !source.empty();
    }

    bool ConfigurationExportOptions::has_exported_at() const noexcept
    {
        return !exported_at.empty();
    }

    bool ConfigurationExportOptions::requests_devices() const noexcept
    {
        return include_devices;
    }

    bool ConfigurationExportOptions::requests_tags() const noexcept
    {
        return include_tags;
    }
}