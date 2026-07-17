#pragma once

#include <dispatcher/domain/tag_definition.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class TagDefinitionBuilder
    {
    public:
        TagDefinitionBuilder& organization_id(OrganizationId value);
        TagDefinitionBuilder& site_id(SiteId value);
        TagDefinitionBuilder& area_id(AreaId value);
        TagDefinitionBuilder& device_id(DeviceId value);
        TagDefinitionBuilder& tag_id(TagId value);

        TagDefinitionBuilder& local_name(std::string value);
        TagDefinitionBuilder& display_name(std::string value);
        TagDefinitionBuilder& name(std::string value);

        TagDefinitionBuilder& description(std::string value);
        TagDefinitionBuilder& data_type(DataType value);
        TagDefinitionBuilder& engineering_unit(std::string value);
        TagDefinitionBuilder& history_policy(HistoryPolicy value);
        TagDefinitionBuilder& deadband(Deadband value);
        TagDefinitionBuilder& scaling(Scaling value);
        TagDefinitionBuilder& enabled(bool value);
        TagDefinitionBuilder& config_version(std::uint64_t value);

        [[nodiscard]] TagDefinition build() const;

    private:
        OrganizationId organization_id_;
        SiteId site_id_;
        AreaId area_id_;
        DeviceId device_id_;
        TagId tag_id_;

        std::string local_name_;
        std::string display_name_;
        std::string description_;
        DataType data_type_{ DataType::Float64 };
        std::string engineering_unit_;
        HistoryPolicy history_policy_{ HistoryPolicy::Disabled };
        Deadband deadband_{ 0.0 };
        Scaling scaling_{ 1.0, 0.0 };
        bool enabled_{ true };
        std::uint64_t config_version_{ 1 };
    };
}