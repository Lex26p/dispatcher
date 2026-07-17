#pragma once

#include <stdexcept>
#include <string>

namespace dispatcher::storage::sqlite
{
    class SqliteError final : public std::runtime_error
    {
    public:
        explicit SqliteError(
            const std::string& message
        )
            : std::runtime_error(
                message
            )
        {
        }
    };
}