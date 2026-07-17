#include <dispatcher/domain/tag_catalog.hpp>

#include <dispatcher/domain/tag_definition_validation.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace
{
    std::string make_device_local_name_key(
        const dispatcher::domain::OrganizationId& organization_id,
        const dispatcher::domain::SiteId& site_id,
        const dispatcher::domain::AreaId& area_id,
        const dispatcher::domain::DeviceId& device_id,
        std::string_view local_name
    )
    {
        return organization_id.value()
            + "/"
            + site_id.value()
            + "/"
            + area_id.value()
            + "/"
            + device_id.value()
            + "/"
            + std::string(local_name);
    }
}

namespace dispatcher::domain
{
    dispatcher::common::ValidationResult TagCatalog::add(TagDefinition tag_definition)
    {
        auto result = validate_tag_definition(tag_definition);

        const auto tag_id = tag_definition.tag_id().value();
        const auto address = make_tag_address(tag_definition).full_name();

        const auto device_local_name_key = make_device_local_name_key(
            tag_definition.organization_id(),
            tag_definition.site_id(),
            tag_definition.area_id(),
            tag_definition.device_id(),
            tag_definition.local_name()
        );

        if (by_id_.contains(tag_id))
        {
            result.add_error(
                "tag_id",
                "tag_id must be unique"
            );
        }

        if (by_address_.contains(address))
        {
            result.add_error(
                "address",
                "tag address must be unique"
            );
        }

        if (by_device_local_name_.contains(device_local_name_key))
        {
            result.add_error(
                "local_name",
                "local_name must be unique within device"
            );
        }

        if (result.has_errors())
        {
            return result;
        }

        const auto index = tags_.size();

        tags_.push_back(std::move(tag_definition));
        by_id_.emplace(tag_id, index);
        by_address_.emplace(address, index);
        by_device_local_name_.emplace(device_local_name_key, index);

        return result;
    }

    std::optional<TagDefinition> TagCatalog::find_by_id(const TagId& tag_id) const
    {
        const auto it = by_id_.find(tag_id.value());

        if (it == by_id_.end())
        {
            return std::nullopt;
        }

        return tags_[it->second];
    }

    std::optional<TagDefinition> TagCatalog::find_by_address(const TagAddress& address) const
    {
        const auto it = by_address_.find(address.full_name());

        if (it == by_address_.end())
        {
            return std::nullopt;
        }

        return tags_[it->second];
    }

    std::optional<TagDefinition> TagCatalog::find_by_device_local_name(
        const OrganizationId& organization_id,
        const SiteId& site_id,
        const AreaId& area_id,
        const DeviceId& device_id,
        std::string_view local_name
    ) const
    {
        const auto key = make_device_local_name_key(
            organization_id,
            site_id,
            area_id,
            device_id,
            local_name
        );

        const auto it = by_device_local_name_.find(key);

        if (it == by_device_local_name_.end())
        {
            return std::nullopt;
        }

        return tags_[it->second];
    }

    bool TagCatalog::contains(const TagId& tag_id) const
    {
        return by_id_.contains(tag_id.value());
    }

    std::size_t TagCatalog::size() const noexcept
    {
        return tags_.size();
    }

    bool TagCatalog::empty() const noexcept
    {
        return tags_.empty();
    }

    const std::vector<TagDefinition>& TagCatalog::tags() const noexcept
    {
        return tags_;
    }
}