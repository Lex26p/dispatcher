#pragma once

#include <string>

namespace dispatcher::api
{
    class TransportError
    {
    public:
        TransportError(
            std::string code = {},
            std::string message = {},
            std::string field = {},
            std::string detail = {}
        );

        [[nodiscard]] static TransportError none();

        [[nodiscard]] static TransportError invalid_request(
            std::string message,
            std::string field = {},
            std::string detail = {}
        );

        [[nodiscard]] static TransportError unauthorized(
            std::string message,
            std::string detail = {}
        );

        [[nodiscard]] static TransportError forbidden(
            std::string message,
            std::string detail = {}
        );

        [[nodiscard]] static TransportError not_found(
            std::string message,
            std::string detail = {}
        );

        [[nodiscard]] static TransportError conflict(
            std::string message,
            std::string detail = {}
        );

        [[nodiscard]] static TransportError internal_error(
            std::string message,
            std::string detail = {}
        );

        [[nodiscard]] const std::string& code() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& detail() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] bool has_code() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_detail() const noexcept;

    private:
        std::string code_;
        std::string message_;
        std::string field_;
        std::string detail_;
    };
}