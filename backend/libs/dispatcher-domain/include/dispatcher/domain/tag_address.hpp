#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <string>

namespace dispatcher::domain
{
    class TagAddress
    {
    public:
        TagAddress(
            OrganizationId organization_id,
            SiteId site_id,
            AreaId area_id,
            DeviceId device_id,
            TagId tag_id
        );

        [[nodiscard]] const OrganizationId& organization_id() const noexcept;
        [[nodiscard]] const SiteId& site_id() const noexcept;
        [[nodiscard]] const AreaId& area_id() const noexcept;
        [[nodiscard]] const DeviceId& device_id() const noexcept;
        [[nodiscard]] const TagId& tag_id() const noexcept;

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::string full_name() const;

    private:
        OrganizationId organization_id_;
        SiteId site_id_;
        AreaId area_id_;
        DeviceId device_id_;
        TagId tag_id_;
    };

    [[nodiscard]] TagAddress make_tag_address(const TagDefinition& tag_definition);
}