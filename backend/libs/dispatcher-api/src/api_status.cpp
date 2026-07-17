#include <dispatcher/api/api_status.hpp>

namespace dispatcher::api
{
    const char* to_string(ApiStatus status) noexcept
    {
        switch (status)
        {
        case ApiStatus::Success:
            return "success";
        case ApiStatus::Accepted:
            return "accepted";
        case ApiStatus::NoContent:
            return "no_content";
        case ApiStatus::BadRequest:
            return "bad_request";
        case ApiStatus::Unauthorized:
            return "unauthorized";
        case ApiStatus::Forbidden:
            return "forbidden";
        case ApiStatus::NotFound:
            return "not_found";
        case ApiStatus::Conflict:
            return "conflict";
        case ApiStatus::ValidationError:
            return "validation_error";
        case ApiStatus::RuntimeRejected:
            return "runtime_rejected";
        case ApiStatus::StorageError:
            return "storage_error";
        case ApiStatus::Timeout:
            return "timeout";
        case ApiStatus::UnsupportedOperation:
            return "unsupported_operation";
        case ApiStatus::InternalError:
            return "internal_error";
        }

        return "internal_error";
    }

    bool is_success(ApiStatus status) noexcept
    {
        return status == ApiStatus::Success
            || status == ApiStatus::Accepted
            || status == ApiStatus::NoContent;
    }

    bool is_failure(ApiStatus status) noexcept
    {
        return !is_success(status);
    }

    bool is_client_error(ApiStatus status) noexcept
    {
        return status == ApiStatus::BadRequest
            || status == ApiStatus::Unauthorized
            || status == ApiStatus::Forbidden
            || status == ApiStatus::NotFound
            || status == ApiStatus::Conflict
            || status == ApiStatus::ValidationError;
    }

    bool is_server_error(ApiStatus status) noexcept
    {
        return status == ApiStatus::RuntimeRejected
            || status == ApiStatus::StorageError
            || status == ApiStatus::Timeout
            || status == ApiStatus::UnsupportedOperation
            || status == ApiStatus::InternalError;
    }
}