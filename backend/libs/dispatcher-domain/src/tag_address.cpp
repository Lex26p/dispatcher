#include <dispatcher/domain/tag_address.hpp>

#include <utility>

namespace dispatcher::domain
{
    TagAddress::TagAddress(
        OrganizationId organization_id,
        SiteId site_id,
        AreaId area_id,
        DeviceId device_id,
        TagId tag_id
    )
        : organization_id_(std::move(organization_id))
        , site_id_(std::move(site_id))
        , area_id_(std::move(area_id))
        , device_id_(std::move(device_id))
        , tag_id_(std::move(tag_id))
    {
    }

    const OrganizationId& TagAddress::organization_id() const noexcept
    {
        return organization_id_;
    }

    const SiteId& TagAddress::site_id() const noexcept
    {
        return site_id_;
    }

    const AreaId& TagAddress::area_id() const noexcept
    {
        return area_id_;
    }

    const DeviceId& TagAddress::device_id() const noexcept
    {
        return device_id_;
    }

    const TagId& TagAddress::tag_id() const noexcept
    {
        return tag_id_;
    }

    bool TagAddress::empty() const noexcept
    {
        return organization_id_.empty()
            || site_id_.empty()
            || area_id_.empty()
            || device_id_.empty()
            || tag_id_.empty();
    }

    std::string TagAddress::full_name() const
    {
        return organization_id_.value()
            + "/"
            + site_id_.value()
            + "/"
            + area_id_.value()
            + "/"
            + device_id_.value()
            + "/"
            + tag_id_.value();
    }

    TagAddress make_tag_address(const TagDefinition& tag_definition)
    {
        return TagAddress(
            tag_definition.organization_id(),
            tag_definition.site_id(),
            tag_definition.area_id(),
            tag_definition.device_id(),
            tag_definition.tag_id()
        );
    }
}