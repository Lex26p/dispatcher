#include <dispatcher/api/api_page.hpp>

#include <algorithm>

namespace dispatcher::api
{
    ApiPage::ApiPage(
        std::size_t offset,
        std::size_t limit,
        std::size_t returned_count,
        std::size_t total_count
    )
        : offset_(offset)
        , limit_(limit)
        , returned_count_(returned_count)
        , total_count_(total_count)
    {
    }

    ApiPage ApiPage::from_request(
        const ApiPageRequest& request,
        std::size_t returned_count,
        std::size_t total_count
    )
    {
        return ApiPage(
            request.offset,
            request.limit,
            returned_count,
            total_count
        );
    }

    std::size_t ApiPage::offset() const noexcept
    {
        return offset_;
    }

    std::size_t ApiPage::limit() const noexcept
    {
        return limit_;
    }

    std::size_t ApiPage::returned_count() const noexcept
    {
        return returned_count_;
    }

    std::size_t ApiPage::total_count() const noexcept
    {
        return total_count_;
    }

    bool ApiPage::empty() const noexcept
    {
        return returned_count_ == 0;
    }

    bool ApiPage::has_next_page() const noexcept
    {
        return limit_ > 0
            && offset_ + returned_count_ < total_count_;
    }

    bool ApiPage::has_previous_page() const noexcept
    {
        return offset_ > 0;
    }

    std::size_t ApiPage::next_offset() const noexcept
    {
        if (!has_next_page())
        {
            return offset_;
        }

        return offset_ + returned_count_;
    }

    std::size_t ApiPage::previous_offset() const noexcept
    {
        if (!has_previous_page())
        {
            return 0;
        }

        if (offset_ <= limit_)
        {
            return 0;
        }

        return offset_ - limit_;
    }
}