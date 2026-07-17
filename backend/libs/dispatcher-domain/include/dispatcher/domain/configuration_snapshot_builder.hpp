#pragma once

#include <dispatcher/common/validation_result.hpp>
#include <dispatcher/domain/configuration_metadata.hpp>
#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/domain/device_catalog.hpp>
#include <dispatcher/domain/device_definition.hpp>
#include <dispatcher/domain/tag_catalog.hpp>
#include <dispatcher/domain/tag_definition.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class ConfigurationSnapshotBuilder
    {
    public:
        ConfigurationSnapshotBuilder& config_version(std::uint64_t value);
        ConfigurationSnapshotBuilder& status(ConfigurationStatus value);
        ConfigurationSnapshotBuilder& description(std::string value);

        ConfigurationSnapshotBuilder& draft();
        ConfigurationSnapshotBuilder& published();

        [[nodiscard]] dispatcher::common::ValidationResult add_device(
            DeviceDefinition device_definition
        );

        [[nodiscard]] dispatcher::common::ValidationResult add_tag(
            TagDefinition tag_definition
        );

        [[nodiscard]] ConfigurationSnapshot build() const;

        [[nodiscard]] dispatcher::common::ValidationResult validate() const;

        [[nodiscard]] const DeviceCatalog& device_catalog() const noexcept;
        [[nodiscard]] const TagCatalog& tag_catalog() const noexcept;

    private:
        std::uint64_t config_version_{ 1 };
        ConfigurationStatus status_{ ConfigurationStatus::Draft };
        std::string description_;

        DeviceCatalog device_catalog_;
        TagCatalog tag_catalog_;
    };
}