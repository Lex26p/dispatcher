#include <dispatcher/api/transport_endpoint.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportEndpoint::TransportEndpoint(
        std::string name,
        TransportMethod method,
        std::string path,
        bool requires_authentication,
        bool streaming,
        std::string description
    )
        : name_(std::move(name))
        , method_(method)
        , path_(std::move(path))
        , requires_authentication_(requires_authentication)
        , streaming_(streaming)
        , description_(std::move(description))
    {
    }

    const std::string& TransportEndpoint::name() const noexcept
    {
        return name_;
    }

    TransportMethod TransportEndpoint::method() const noexcept
    {
        return method_;
    }

    const std::string& TransportEndpoint::path() const noexcept
    {
        return path_;
    }

    bool TransportEndpoint::requires_authentication() const noexcept
    {
        return requires_authentication_;
    }

    bool TransportEndpoint::public_endpoint() const noexcept
    {
        return !requires_authentication_;
    }

    bool TransportEndpoint::streaming() const noexcept
    {
        return streaming_;
    }

    const std::string& TransportEndpoint::description() const noexcept
    {
        return description_;
    }

    bool TransportEndpoint::has_name() const noexcept
    {
        return !name_.empty();
    }

    bool TransportEndpoint::has_path() const noexcept
    {
        return !path_.empty();
    }

    bool TransportEndpoint::has_description() const noexcept
    {
        return !description_.empty();
    }

    bool TransportEndpoint::valid() const noexcept
    {
        return has_name()
            && is_known_method(method_)
            && has_path()
            && path_.front() == '/';
    }

    bool TransportEndpoint::matches(
        TransportMethod method,
        const std::string& path
    ) const noexcept
    {
        return method_ == method
            && path_ == path;
    }

    bool TransportEndpoint::compatible_with(
        TransportProtocol protocol
    ) const noexcept
    {
        if (!is_known_protocol(protocol))
        {
            return false;
        }

        if (streaming_)
        {
            return supports_streaming(protocol);
        }

        return true;
    }

    std::string TransportEndpoint::key() const
    {
        return make_key(
            method_,
            path_
        );
    }

    std::string TransportEndpoint::make_key(
        TransportMethod method,
        const std::string& path
    )
    {
        std::string result = to_string(method);

        result += " ";
        result += path;

        return result;
    }
}