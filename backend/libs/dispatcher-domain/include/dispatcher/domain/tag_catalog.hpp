#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/tag_address.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dispatcher::domain
{
    class TagCatalog
    {
    public:
        [[nodiscard]] dispatcher::common::ValidationResult add(TagDefinition tag_definition);

        [[nodiscard]] std::optional<TagDefinition> find_by_id(const TagId& tag_id) const;
        [[nodiscard]] std::optional<TagDefinition> find_by_address(const TagAddress& address) const;

        [[nodiscard]] std::optional<TagDefinition> find_by_device_local_name(
            const OrganizationId& organization_id,
            const SiteId& site_id,
            const AreaId& area_id,
            const DeviceId& device_id,
            std::string_view local_name
        ) const;

        [[nodiscard]] bool contains(const TagId& tag_id) const;

        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] const std::vector<TagDefinition>& tags() const noexcept;

    private:
        std::vector<TagDefinition> tags_;
        std::unordered_map<std::string, std::size_t> by_id_;
        std::unordered_map<std::string, std::size_t> by_address_;
        std::unordered_map<std::string, std::size_t> by_device_local_name_;
    };
}