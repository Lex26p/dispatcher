#pragma once

#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_request_context.hpp>

#include <string>

namespace dispatcher::api
{
    class TransportRequest
    {
    public:
        TransportRequest(
            TransportRequestContext context,
            std::string body = {},
            std::string content_type = {}
        );

        [[nodiscard]] const TransportRequestContext& context() const noexcept;

        [[nodiscard]] TransportProtocol protocol() const noexcept;

        [[nodiscard]] TransportMethod method() const noexcept;

        [[nodiscard]] const std::string& method_text() const noexcept;

        [[nodiscard]] const std::string& path() const noexcept;

        [[nodiscard]] const std::string& body() const noexcept;

        [[nodiscard]] const std::string& content_type() const noexcept;

        [[nodiscard]] bool has_body() const noexcept;

        [[nodiscard]] bool has_content_type() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

    private:
        TransportRequestContext context_;
        std::string body_;
        std::string content_type_;
    };
}