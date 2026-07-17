#pragma once

#include <dispatcher/http/http_server_status.hpp>

#include <string>

namespace dispatcher::http
{
    class HttpServerResult
    {
    public:
        HttpServerResult(
            bool ok,
            HttpServerStatus status,
            std::string message = {}
        );

        [[nodiscard]] static HttpServerResult success(
            HttpServerStatus status,
            std::string message = {}
        );

        [[nodiscard]] static HttpServerResult failure(
            HttpServerStatus status,
            std::string message
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] HttpServerStatus status() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

    private:
        bool ok_{ false };
        HttpServerStatus status_{ HttpServerStatus::Unknown };
        std::string message_;
    };
}