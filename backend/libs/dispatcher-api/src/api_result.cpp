#include <dispatcher/api/api_result.hpp>

#include <utility>

namespace dispatcher::api
{
    ApiResult ApiResult::success()
    {
        return ApiResult(
            ApiError{
                .status = ApiStatus::Success
            }
        );
    }

    ApiResult ApiResult::accepted()
    {
        return ApiResult(
            ApiError{
                .status = ApiStatus::Accepted
            }
        );
    }

    ApiResult ApiResult::no_content()
    {
        return ApiResult(
            ApiError{
                .status = ApiStatus::NoContent
            }
        );
    }

    ApiResult ApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        if (is_success(status))
        {
            status = ApiStatus::InternalError;
        }

        return ApiResult(
            ApiError{
                .status = status,
                .operation = std::move(operation),
                .resource = std::move(resource),
                .field = std::move(field),
                .message = std::move(message)
            }
        );
    }

    bool ApiResult::ok() const noexcept
    {
        return is_success(error_.status);
    }

    bool ApiResult::failed() const noexcept
    {
        return !ok();
    }

    ApiStatus ApiResult::status() const noexcept
    {
        return error_.status;
    }

    const ApiError& ApiResult::error() const noexcept
    {
        return error_;
    }

    const std::string& ApiResult::operation() const noexcept
    {
        return error_.operation;
    }

    const std::string& ApiResult::resource() const noexcept
    {
        return error_.resource;
    }

    const std::string& ApiResult::field() const noexcept
    {
        return error_.field;
    }

    const std::string& ApiResult::message() const noexcept
    {
        return error_.message;
    }

    ApiResult::ApiResult(ApiError error)
        : error_(std::move(error))
    {
    }
}