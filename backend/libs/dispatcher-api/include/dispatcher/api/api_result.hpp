#pragma once

#include <dispatcher/api/api_error.hpp>
#include <dispatcher/api/api_status.hpp>

#include <string>

namespace dispatcher::api
{
    class ApiResult
    {
    public:
        [[nodiscard]] static ApiResult success();

        [[nodiscard]] static ApiResult accepted();

        [[nodiscard]] static ApiResult no_content();

        [[nodiscard]] static ApiResult failure(
            ApiStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ApiStatus status() const noexcept;

        [[nodiscard]] const ApiError& error() const noexcept;

        [[nodiscard]] const std::string& operation() const noexcept;

        [[nodiscard]] const std::string& resource() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

    private:
        explicit ApiResult(ApiError error);

        ApiError error_;
    };
}