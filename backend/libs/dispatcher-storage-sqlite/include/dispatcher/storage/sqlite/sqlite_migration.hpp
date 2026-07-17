#pragma once

#include <cstdint>
#include <string>

namespace dispatcher::storage::sqlite
{
    struct SqliteMigration
    {
        std::int64_t version{ 0 };
        std::string name{};
        std::string sql{};
    };
}