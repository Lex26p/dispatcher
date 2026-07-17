#include <dispatcher/api/api_page_request.hpp>

namespace dispatcher::api
{
    bool ApiPageRequest::has_offset() const noexcept
    {
        return offset > 0;
    }

    bool ApiPageRequest::has_limit() const noexcept
    {
        return limit > 0;
    }

    std::size_t ApiPageRequest::start_index() const noexcept
    {
        return offset;
    }

    std::size_t ApiPageRequest::end_index_exclusive() const noexcept
    {
        return offset + limit;
    }
}