#include <dispatcher/api/transport_response.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportResponse::TransportResponse(
        TransportStatus status,
        std::string body,
        std::string content_type,
        Headers headers,
        std::optional<TransportError> error
    )
        : status_(status)
        , body_(std::move(body))
        , content_type_(std::move(content_type))
        , headers_(std::move(headers))
        , error_(std::move(error))
    {
    }

    TransportResponse TransportResponse::success(
        std::string body,
        std::string content_type
    )
    {
        return TransportResponse(
            TransportStatus::Ok,
            std::move(body),
            std::move(content_type)
        );
    }

    TransportResponse TransportResponse::created(
        std::string body,
        std::string content_type
    )
    {
        return TransportResponse(
            TransportStatus::Created,
            std::move(body),
            std::move(content_type)
        );
    }

    TransportResponse TransportResponse::accepted(
        std::string body,
        std::string content_type
    )
    {
        return TransportResponse(
            TransportStatus::Accepted,
            std::move(body),
            std::move(content_type)
        );
    }

    TransportResponse TransportResponse::failure(
        TransportStatus status,
        TransportError error,
        std::string body,
        std::string content_type
    )
    {
        if (is_success(status))
        {
            status = TransportStatus::InternalError;
        }

        return TransportResponse(
            status,
            std::move(body),
            std::move(content_type),
            {},
            std::move(error)
        );
    }

    TransportStatus TransportResponse::status() const noexcept
    {
        return status_;
    }

    std::uint16_t TransportResponse::http_status() const noexcept
    {
        return http_status_code(status_);
    }

    std::uint16_t TransportResponse::grpc_status() const noexcept
    {
        return grpc_status_code(status_);
    }

    const std::string& TransportResponse::body() const noexcept
    {
        return body_;
    }

    const std::string& TransportResponse::content_type() const noexcept
    {
        return content_type_;
    }

    const TransportResponse::Headers& TransportResponse::headers()
        const noexcept
    {
        return headers_;
    }

    const std::optional<TransportError>& TransportResponse::error()
        const noexcept
    {
        return error_;
    }

    bool TransportResponse::ok() const noexcept
    {
        return is_success(status_);
    }

    bool TransportResponse::failed() const noexcept
    {
        return !ok();
    }

    bool TransportResponse::has_body() const noexcept
    {
        return !body_.empty();
    }

    bool TransportResponse::has_content_type() const noexcept
    {
        return !content_type_.empty();
    }

    bool TransportResponse::has_headers() const noexcept
    {
        return !headers_.empty();
    }

    bool TransportResponse::has_header(
        const std::string& name
    ) const
    {
        return headers_.contains(name);
    }

    std::optional<std::string> TransportResponse::header(
        const std::string& name
    ) const
    {
        const auto iterator = headers_.find(name);

        if (iterator == headers_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    std::size_t TransportResponse::header_count() const noexcept
    {
        return headers_.size();
    }

    bool TransportResponse::has_error() const noexcept
    {
        return error_.has_value()
            && !error_->empty();
    }
}