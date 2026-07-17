#pragma once

#include <dispatcher/api/api_page_request.hpp>

#include <cstddef>

namespace dispatcher::api
{
    class ApiPage
    {
    public:
        ApiPage() = default;

        ApiPage(
            std::size_t offset,
            std::size_t limit,
            std::size_t returned_count,
            std::size_t total_count
        );

        [[nodiscard]] static ApiPage from_request(
            const ApiPageRequest& request,
            std::size_t returned_count,
            std::size_t total_count
        );

        [[nodiscard]] std::size_t offset() const noexcept;

        [[nodiscard]] std::size_t limit() const noexcept;

        [[nodiscard]] std::size_t returned_count() const noexcept;

        [[nodiscard]] std::size_t total_count() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_next_page() const noexcept;

        [[nodiscard]] bool has_previous_page() const noexcept;

        [[nodiscard]] std::size_t next_offset() const noexcept;

        [[nodiscard]] std::size_t previous_offset() const noexcept;

    private:
        std::size_t offset_{ 0 };
        std::size_t limit_{ 100 };
        std::size_t returned_count_{ 0 };
        std::size_t total_count_{ 0 };
    };
}