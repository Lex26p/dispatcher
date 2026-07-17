#include <dispatcher/domain/tag_definition.hpp>

#include <utility>

namespace dispatcher::domain
{
    TagDefinition::TagDefinition(
        OrganizationId organization_id,
        SiteId site_id,
        AreaId area_id,
        DeviceId device_id,
        TagId tag_id,
        std::string local_name,
        std::string description,
        DataType data_type,
        std::string engineering_unit,
        HistoryPolicy history_policy,
        Deadband deadband,
        Scaling scaling,
        bool enabled,
        std::uint64_t config_version,
        std::string display_name
    )
        : organization_id_(std::move(organization_id))
        , site_id_(std::move(site_id))
        , area_id_(std::move(area_id))
        , device_id_(std::move(device_id))
        , tag_id_(std::move(tag_id))
        , local_name_(std::move(local_name))
        , display_name_(std::move(display_name))
        , description_(std::move(description))
        , data_type_(data_type)
        , engineering_unit_(std::move(engineering_unit))
        , history_policy_(history_policy)
        , deadband_(deadband)
        , scaling_(scaling)
        , enabled_(enabled)
        , config_version_(config_version)
    {
    }

    const OrganizationId& TagDefinition::organization_id() const noexcept
    {
        return organization_id_;
    }

    const SiteId& TagDefinition::site_id() const noexcept
    {
        return site_id_;
    }

    const AreaId& TagDefinition::area_id() const noexcept
    {
        return area_id_;
    }

    const DeviceId& TagDefinition::device_id() const noexcept
    {
        return device_id_;
    }

    const TagId& TagDefinition::tag_id() const noexcept
    {
        return tag_id_;
    }

    const std::string& TagDefinition::local_name() const noexcept
    {
        return local_name_;
    }

    const std::string& TagDefinition::display_name() const noexcept
    {
        return display_name_;
    }

    const std::string& TagDefinition::name() const noexcept
    {
        return local_name_;
    }

    const std::string& TagDefinition::description() const noexcept
    {
        return description_;
    }

    DataType TagDefinition::data_type() const noexcept
    {
        return data_type_;
    }

    const std::string& TagDefinition::engineering_unit() const noexcept
    {
        return engineering_unit_;
    }

    HistoryPolicy TagDefinition::history_policy() const noexcept
    {
        return history_policy_;
    }

    Deadband TagDefinition::deadband() const noexcept
    {
        return deadband_;
    }

    Scaling TagDefinition::scaling() const noexcept
    {
        return scaling_;
    }

    bool TagDefinition::enabled() const noexcept
    {
        return enabled_;
    }

    std::uint64_t TagDefinition::config_version() const noexcept
    {
        return config_version_;
    }
}