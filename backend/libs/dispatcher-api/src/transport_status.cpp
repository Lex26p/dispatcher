#include <dispatcher/api/transport_status.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportStatus status) noexcept
    {
        switch (status)
        {
        case TransportStatus::Ok:
            return "ok";

        case TransportStatus::Created:
            return "created";

        case TransportStatus::Accepted:
            return "accepted";

        case TransportStatus::BadRequest:
            return "bad_request";

        case TransportStatus::Unauthorized:
            return "unauthorized";

        case TransportStatus::Forbidden:
            return "forbidden";

        case TransportStatus::NotFound:
            return "not_found";

        case TransportStatus::Conflict:
            return "conflict";

        case TransportStatus::UnprocessableEntity:
            return "unprocessable_entity";

        case TransportStatus::InternalError:
            return "internal_error";

        case TransportStatus::Unavailable:
            return "unavailable";
        }

        return "internal_error";
    }

    std::uint16_t http_status_code(TransportStatus status) noexcept
    {
        switch (status)
        {
        case TransportStatus::Ok:
            return 200;

        case TransportStatus::Created:
            return 201;

        case TransportStatus::Accepted:
            return 202;

        case TransportStatus::BadRequest:
            return 400;

        case TransportStatus::Unauthorized:
            return 401;

        case TransportStatus::Forbidden:
            return 403;

        case TransportStatus::NotFound:
            return 404;

        case TransportStatus::Conflict:
            return 409;

        case TransportStatus::UnprocessableEntity:
            return 422;

        case TransportStatus::InternalError:
            return 500;

        case TransportStatus::Unavailable:
            return 503;
        }

        return 500;
    }

    std::uint16_t grpc_status_code(TransportStatus status) noexcept
    {
        switch (status)
        {
        case TransportStatus::Ok:
        case TransportStatus::Created:
        case TransportStatus::Accepted:
            return 0;

        case TransportStatus::BadRequest:
        case TransportStatus::UnprocessableEntity:
            return 3;

        case TransportStatus::NotFound:
            return 5;

        case TransportStatus::Conflict:
            return 6;

        case TransportStatus::Forbidden:
            return 7;

        case TransportStatus::InternalError:
            return 13;

        case TransportStatus::Unavailable:
            return 14;

        case TransportStatus::Unauthorized:
            return 16;
        }

        return 13;
    }

    bool is_success(TransportStatus status) noexcept
    {
        switch (status)
        {
        case TransportStatus::Ok:
        case TransportStatus::Created:
        case TransportStatus::Accepted:
            return true;

        case TransportStatus::BadRequest:
        case TransportStatus::Unauthorized:
        case TransportStatus::Forbidden:
        case TransportStatus::NotFound:
        case TransportStatus::Conflict:
        case TransportStatus::UnprocessableEntity:
        case TransportStatus::InternalError:
        case TransportStatus::Unavailable:
            return false;
        }

        return false;
    }

    bool is_client_error(TransportStatus status) noexcept
    {
        switch (status)
        {
        case TransportStatus::BadRequest:
        case TransportStatus::Unauthorized:
        case TransportStatus::Forbidden:
        case TransportStatus::NotFound:
        case TransportStatus::Conflict:
        case TransportStatus::UnprocessableEntity:
            return true;

        case TransportStatus::Ok:
        case TransportStatus::Created:
        case TransportStatus::Accepted:
        case TransportStatus::InternalError:
        case TransportStatus::Unavailable:
            return false;
        }

        return false;
    }

    bool is_server_error(TransportStatus status) noexcept
    {
        return status == TransportStatus::InternalError
            || status == TransportStatus::Unavailable;
    }

    bool is_failure(TransportStatus status) noexcept
    {
        return !is_success(status);
    }
}