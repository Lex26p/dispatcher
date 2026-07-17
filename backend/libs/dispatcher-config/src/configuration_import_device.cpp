#include <dispatcher/config/configuration_import_device.hpp>

namespace dispatcher::config
{
    bool ConfigurationImportDevice::has_organization_id() const noexcept
    {
        return !organization_id.empty();
    }

    bool ConfigurationImportDevice::has_site_id() const noexcept
    {
        return !site_id.empty();
    }

    bool ConfigurationImportDevice::has_area_id() const noexcept
    {
        return !area_id.empty();
    }

    bool ConfigurationImportDevice::has_device_id() const noexcept
    {
        return !device_id.empty();
    }

    bool ConfigurationImportDevice::has_local_name() const noexcept
    {
        return !local_name.empty();
    }

    bool ConfigurationImportDevice::has_display_name() const noexcept
    {
        return !display_name.empty();
    }

    bool ConfigurationImportDevice::has_required_identity() const noexcept
    {
        return has_organization_id()
            && has_site_id()
            && has_area_id()
            && has_device_id();
    }
}