#include <dispatcher/api/transport_request.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportRequest::TransportRequest(
        TransportRequestContext context,
        std::string body,
        std::string content_type
    )
        : context_(std::move(context))
        , body_(std::move(body))
        , content_type_(std::move(content_type))
    {
    }

    const TransportRequestContext& TransportRequest::context() const noexcept
    {
        return context_;
    }

    TransportProtocol TransportRequest::protocol() const noexcept
    {
        return context_.protocol();
    }

    TransportMethod TransportRequest::method() const noexcept
    {
        return transport_method_from_string(
            context_.method()
        );
    }

    const std::string& TransportRequest::method_text() const noexcept
    {
        return context_.method();
    }

    const std::string& TransportRequest::path() const noexcept
    {
        return context_.path();
    }

    const std::string& TransportRequest::body() const noexcept
    {
        return body_;
    }

    const std::string& TransportRequest::content_type() const noexcept
    {
        return content_type_;
    }

    bool TransportRequest::has_body() const noexcept
    {
        return !body_.empty();
    }

    bool TransportRequest::has_content_type() const noexcept
    {
        return !content_type_.empty();
    }

    bool TransportRequest::valid() const noexcept
    {
        return context_.valid()
            && is_known_method(method());
    }
}