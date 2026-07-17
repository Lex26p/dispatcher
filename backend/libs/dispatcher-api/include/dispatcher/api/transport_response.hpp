#pragma once

#include <dispatcher/api/transport_error.hpp>
#include <dispatcher/api/transport_status.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace dispatcher::api
{
    class TransportResponse
    {
    public:
        using Headers = std::unordered_map<std::string, std::string>;

        TransportResponse(
            TransportStatus status,
            std::string body = {},
            std::string content_type = {},
            Headers headers = {},
            std::optional<TransportError> error = std::nullopt
        );

        [[nodiscard]] static TransportResponse success(
            std::string body = {},
            std::string content_type = "application/json"
        );

        [[nodiscard]] static TransportResponse created(
            std::string body = {},
            std::string content_type = "application/json"
        );

        [[nodiscard]] static TransportResponse accepted(
            std::string body = {},
            std::string content_type = "application/json"
        );

        [[nodiscard]] static TransportResponse failure(
            TransportStatus status,
            TransportError error,
            std::string body = {},
            std::string content_type = "application/json"
        );

        [[nodiscard]] TransportStatus status() const noexcept;

        [[nodiscard]] std::uint16_t http_status() const noexcept;

        [[nodiscard]] std::uint16_t grpc_status() const noexcept;

        [[nodiscard]] const std::string& body() const noexcept;

        [[nodiscard]] const std::string& content_type() const noexcept;

        [[nodiscard]] const Headers& headers() const noexcept;

        [[nodiscard]] const std::optional<TransportError>& error()
            const noexcept;

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] bool has_body() const noexcept;

        [[nodiscard]] bool has_content_type() const noexcept;

        [[nodiscard]] bool has_headers() const noexcept;

        [[nodiscard]] bool has_header(
            const std::string& name
        ) const;

        [[nodiscard]] std::optional<std::string> header(
            const std::string& name
        ) const;

        [[nodiscard]] std::size_t header_count() const noexcept;

        [[nodiscard]] bool has_error() const noexcept;

    private:
        TransportStatus status_{ TransportStatus::InternalError };
        std::string body_;
        std::string content_type_;
        Headers headers_;
        std::optional<TransportError> error_;
    };
}