#include <dispatcher/config/configuration_export_tag.hpp>

namespace dispatcher::config
{
    bool ConfigurationExportTag::has_organization_id() const noexcept
    {
        return !organization_id.empty();
    }

    bool ConfigurationExportTag::has_site_id() const noexcept
    {
        return !site_id.empty();
    }

    bool ConfigurationExportTag::has_area_id() const noexcept
    {
        return !area_id.empty();
    }

    bool ConfigurationExportTag::has_device_id() const noexcept
    {
        return !device_id.empty();
    }

    bool ConfigurationExportTag::has_tag_id() const noexcept
    {
        return !tag_id.empty();
    }

    bool ConfigurationExportTag::has_local_name() const noexcept
    {
        return !local_name.empty();
    }

    bool ConfigurationExportTag::has_display_name() const noexcept
    {
        return !display_name.empty();
    }

    bool ConfigurationExportTag::has_data_type() const noexcept
    {
        return !data_type.empty();
    }

    bool ConfigurationExportTag::has_history_policy() const noexcept
    {
        return !history_policy.empty();
    }

    bool ConfigurationExportTag::has_required_identity() const noexcept
    {
        return has_organization_id()
            && has_site_id()
            && has_area_id()
            && has_device_id()
            && has_tag_id();
    }
}