#pragma once

#include <stdexcept>
#include <string>

namespace dispatcher::notification::delivery
{
    class NotificationDeliveryError final : public std::runtime_error
    {
    public:
        explicit NotificationDeliveryError(
            const std::string& message
        )
            : std::runtime_error(
                message
            )
        {
        }
    };
}