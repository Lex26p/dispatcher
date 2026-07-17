#include <dispatcher/domain/tag_definition_builder.hpp>

#include <utility>

namespace dispatcher::domain
{
    TagDefinitionBuilder& TagDefinitionBuilder::organization_id(OrganizationId value)
    {
        organization_id_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::site_id(SiteId value)
    {
        site_id_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::area_id(AreaId value)
    {
        area_id_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::device_id(DeviceId value)
    {
        device_id_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::tag_id(TagId value)
    {
        tag_id_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::local_name(std::string value)
    {
        local_name_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::display_name(std::string value)
    {
        display_name_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::name(std::string value)
    {
        return local_name(std::move(value));
    }

    TagDefinitionBuilder& TagDefinitionBuilder::description(std::string value)
    {
        description_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::data_type(DataType value)
    {
        data_type_ = value;
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::engineering_unit(std::string value)
    {
        engineering_unit_ = std::move(value);
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::history_policy(HistoryPolicy value)
    {
        history_policy_ = value;
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::deadband(Deadband value)
    {
        deadband_ = value;
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::scaling(Scaling value)
    {
        scaling_ = value;
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::enabled(bool value)
    {
        enabled_ = value;
        return *this;
    }

    TagDefinitionBuilder& TagDefinitionBuilder::config_version(std::uint64_t value)
    {
        config_version_ = value;
        return *this;
    }

    TagDefinition TagDefinitionBuilder::build() const
    {
        return TagDefinition(
            organization_id_,
            site_id_,
            area_id_,
            device_id_,
            tag_id_,
            local_name_,
            description_,
            data_type_,
            engineering_unit_,
            history_policy_,
            deadband_,
            scaling_,
            enabled_,
            config_version_,
            display_name_
        );
    }
}