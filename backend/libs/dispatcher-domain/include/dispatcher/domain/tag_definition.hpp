#pragma once

#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/deadband.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/scaling.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::domain
{
    class TagDefinition
    {
    public:
        TagDefinition(
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
            std::string display_name = {}
        );

        [[nodiscard]] const OrganizationId& organization_id() const noexcept;
        [[nodiscard]] const SiteId& site_id() const noexcept;
        [[nodiscard]] const AreaId& area_id() const noexcept;
        [[nodiscard]] const DeviceId& device_id() const noexcept;
        [[nodiscard]] const TagId& tag_id() const noexcept;

        [[nodiscard]] const std::string& local_name() const noexcept;
        [[nodiscard]] const std::string& display_name() const noexcept;
        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] const std::string& description() const noexcept;
        [[nodiscard]] DataType data_type() const noexcept;
        [[nodiscard]] const std::string& engineering_unit() const noexcept;
        [[nodiscard]] HistoryPolicy history_policy() const noexcept;
        [[nodiscard]] Deadband deadband() const noexcept;
        [[nodiscard]] Scaling scaling() const noexcept;
        [[nodiscard]] bool enabled() const noexcept;
        [[nodiscard]] std::uint64_t config_version() const noexcept;

    private:
        OrganizationId organization_id_;
        SiteId site_id_;
        AreaId area_id_;
        DeviceId device_id_;
        TagId tag_id_;

        std::string local_name_;
        std::string display_name_;
        std::string description_;
        DataType data_type_;
        std::string engineering_unit_;
        HistoryPolicy history_policy_;
        Deadband deadband_;
        Scaling scaling_;
        bool enabled_;
        std::uint64_t config_version_;
    };
}