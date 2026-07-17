#pragma once

#include <dispatcher/domain/configuration_metadata.hpp>
#include <dispatcher/domain/device_catalog.hpp>
#include <dispatcher/domain/tag_catalog.hpp>
#include <dispatcher/domain/device_definition.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace dispatcher::domain
{
    class ConfigurationSnapshot
    {
    public:
        ConfigurationSnapshot(
            std::uint64_t config_version,
            DeviceCatalog device_catalog,
            TagCatalog tag_catalog
        );

        ConfigurationSnapshot(
            ConfigurationMetadata metadata,
            DeviceCatalog device_catalog,
            TagCatalog tag_catalog
        );

        [[nodiscard]] const ConfigurationMetadata& metadata() const noexcept;

        [[nodiscard]] std::uint64_t config_version() const noexcept;
        [[nodiscard]] ConfigurationStatus status() const noexcept;

        [[nodiscard]] bool is_draft() const noexcept;
        [[nodiscard]] bool is_published() const noexcept;

        [[nodiscard]] const DeviceCatalog& device_catalog() const noexcept;
        [[nodiscard]] const TagCatalog& tag_catalog() const noexcept;

        [[nodiscard]] std::size_t device_count() const noexcept;
        [[nodiscard]] std::size_t tag_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] std::optional<DeviceDefinition> find_device_by_id(
            const DeviceId& device_id
        ) const;

        [[nodiscard]] std::optional<TagDefinition> find_tag_by_id(
            const TagId& tag_id
        ) const;

        [[nodiscard]] const std::vector<DeviceDefinition>& devices()
            const noexcept;

        [[nodiscard]] const std::vector<TagDefinition>& tags()
            const noexcept;

    private:
        ConfigurationMetadata metadata_;
        DeviceCatalog device_catalog_;
        TagCatalog tag_catalog_;
    };
}