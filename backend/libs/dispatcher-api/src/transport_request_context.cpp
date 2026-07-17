#include <dispatcher/api/transport_request_context.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportRequestContext::TransportRequestContext(
        TransportProtocol protocol,
        std::string method,
        std::string path,
        std::string correlation_id,
        std::string operator_id,
        std::string remote_address,
        Headers headers
    )
        : protocol_(protocol)
        , method_(std::move(method))
        , path_(std::move(path))
        , correlation_id_(std::move(correlation_id))
        , operator_id_(std::move(operator_id))
        , remote_address_(std::move(remote_address))
        , headers_(std::move(headers))
    {
    }

    TransportProtocol TransportRequestContext::protocol() const noexcept
    {
        return protocol_;
    }

    const std::string& TransportRequestContext::method() const noexcept
    {
        return method_;
    }

    const std::string& TransportRequestContext::path() const noexcept
    {
        return path_;
    }

    const std::string& TransportRequestContext::correlation_id() const noexcept
    {
        return correlation_id_;
    }

    const std::string& TransportRequestContext::operator_id() const noexcept
    {
        return operator_id_;
    }

    const std::string& TransportRequestContext::remote_address() const noexcept
    {
        return remote_address_;
    }

    const TransportRequestContext::Headers&
        TransportRequestContext::headers() const noexcept
    {
        return headers_;
    }

    bool TransportRequestContext::has_correlation_id() const noexcept
    {
        return !correlation_id_.empty();
    }

    bool TransportRequestContext::has_operator_id() const noexcept
    {
        return !operator_id_.empty();
    }

    bool TransportRequestContext::has_remote_address() const noexcept
    {
        return !remote_address_.empty();
    }

    bool TransportRequestContext::has_headers() const noexcept
    {
        return !headers_.empty();
    }

    bool TransportRequestContext::has_header(
        const std::string& name
    ) const
    {
        return headers_.contains(name);
    }

    std::optional<std::string> TransportRequestContext::header(
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

    std::size_t TransportRequestContext::header_count() const noexcept
    {
        return headers_.size();
    }

    bool TransportRequestContext::valid() const noexcept
    {
        return is_known_protocol(protocol_)
            && !method_.empty()
            && !path_.empty();
    }
}