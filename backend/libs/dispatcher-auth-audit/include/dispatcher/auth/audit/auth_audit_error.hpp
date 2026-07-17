#pragma once

#include <stdexcept>
#include <string>

namespace dispatcher::auth::audit
{
    class AuthAuditError final : public std::runtime_error
    {
    public:
        explicit AuthAuditError(
            const std::string& message
        )
            : std::runtime_error(
                message
            )
        {
        }
    };
}