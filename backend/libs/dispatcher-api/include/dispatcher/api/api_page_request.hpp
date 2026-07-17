#pragma once

#include <cstddef>

namespace dispatcher::api
{
    struct ApiPageRequest
    {
        std::size_t offset{ 0 };
        std::size_t limit{ 100 };

        [[nodiscard]] bool has_offset() const noexcept;

        [[nodiscard]] bool has_limit() const noexcept;

        [[nodiscard]] std::size_t start_index() const noexcept;

        [[nodiscard]] std::size_t end_index_exclusive() const noexcept;
    };
}