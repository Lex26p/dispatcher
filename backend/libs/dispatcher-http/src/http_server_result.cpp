#include <dispatcher/http/http_server_result.hpp>

#include <utility>

namespace dispatcher::http
{
    HttpServerResult::HttpServerResult(
        bool ok,
        HttpServerStatus status,
        std::string message
    )
        : ok_(ok)
        , status_(status)
        , message_(std::move(message))
    {
    }

    HttpServerResult HttpServerResult::success(
        HttpServerStatus status,
        std::string message
    )
    {
        return HttpServerResult(
            true,
            status,
            std::move(message)
        );
    }

    HttpServerResult HttpServerResult::failure(
        HttpServerStatus status,
        std::string message
    )
    {
        return HttpServerResult(
            false,
            status,
            std::move(message)
        );
    }

    bool HttpServerResult::ok() const noexcept
    {
        return ok_;
    }

    bool HttpServerResult::failed() const noexcept
    {
        return !ok_;
    }

    HttpServerStatus HttpServerResult::status() const noexcept
    {
        return status_;
    }

    const std::string& HttpServerResult::message() const noexcept
    {
        return message_;
    }

    bool HttpServerResult::has_message() const noexcept
    {
        return !message_.empty();
    }
}