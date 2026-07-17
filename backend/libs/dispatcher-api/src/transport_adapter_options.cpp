#include <dispatcher/api/transport_adapter_options.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportAdapterOptions::TransportAdapterOptions(
        TransportProtocol protocol,
        std::string bind_address,
        std::uint16_t port,
        bool enabled,
        Timeout request_timeout
    )
        : protocol_(protocol)
        , bind_address_(std::move(bind_address))
        , port_(port)
        , enabled_(enabled)
        , request_timeout_(request_timeout)
    {
    }

    TransportProtocol TransportAdapterOptions::protocol() const noexcept
    {
        return protocol_;
    }

    const std::string& TransportAdapterOptions::bind_address() const noexcept
    {
        return bind_address_;
    }

    std::uint16_t TransportAdapterOptions::port() const noexcept
    {
        return port_;
    }

    bool TransportAdapterOptions::enabled() const noexcept
    {
        return enabled_;
    }

    bool TransportAdapterOptions::disabled() const noexcept
    {
        return !enabled_;
    }

    TransportAdapterOptions::Timeout
        TransportAdapterOptions::request_timeout() const noexcept
    {
        return request_timeout_;
    }

    bool TransportAdapterOptions::has_bind_address() const noexcept
    {
        return !bind_address_.empty();
    }

    bool TransportAdapterOptions::has_port() const noexcept
    {
        return port_ > 0;
    }

    bool TransportAdapterOptions::valid() const noexcept
    {
        return enabled_
            && is_known_protocol(protocol_)
            && has_bind_address()
            && request_timeout_ > Timeout::zero();
    }
}