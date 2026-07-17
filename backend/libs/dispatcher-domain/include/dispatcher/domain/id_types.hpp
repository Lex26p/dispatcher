#pragma once

#include <compare>
#include <string>
#include <utility>

namespace dispatcher::domain
{
    template <typename Tag>
    class StrongId
    {
    public:
        StrongId() = default;

        explicit StrongId(std::string value)
            : value_(std::move(value))
        {
        }

        [[nodiscard]] const std::string& value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return value_.empty();
        }

        [[nodiscard]] auto operator<=>(const StrongId&) const = default;

    private:
        std::string value_;
    };

    struct OrganizationIdTag;
    struct SiteIdTag;
    struct AreaIdTag;
    struct DeviceIdTag;
    struct TagIdTag;
    struct AlarmIdTag;
    struct OperatorIdTag;
    struct OperatorSessionIdTag;
    struct NotificationIdTag;
    struct NotificationRouteIdTag;

    using OrganizationId = StrongId<OrganizationIdTag>;
    using SiteId = StrongId<SiteIdTag>;
    using AreaId = StrongId<AreaIdTag>;
    using DeviceId = StrongId<DeviceIdTag>;
    using TagId = StrongId<TagIdTag>;
    using AlarmId = StrongId<AlarmIdTag>;
    using OperatorId = StrongId<OperatorIdTag>;
    using OperatorSessionId = StrongId<OperatorSessionIdTag>;
    using NotificationId = StrongId<NotificationIdTag>;
    using NotificationRouteId = StrongId<NotificationRouteIdTag>;
}