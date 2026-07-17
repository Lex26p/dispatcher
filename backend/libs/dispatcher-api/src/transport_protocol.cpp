#include <dispatcher/api/transport_protocol.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportProtocol protocol) noexcept
    {
        switch (protocol)
        {
        case TransportProtocol::Unknown:
            return "unknown";

        case TransportProtocol::Http:
            return "http";

        case TransportProtocol::Grpc:
            return "grpc";
        }

        return "unknown";
    }

    bool is_known_protocol(TransportProtocol protocol) noexcept
    {
        return protocol != TransportProtocol::Unknown;
    }

    bool is_http_protocol(TransportProtocol protocol) noexcept
    {
        return protocol == TransportProtocol::Http;
    }

    bool is_grpc_protocol(TransportProtocol protocol) noexcept
    {
        return protocol == TransportProtocol::Grpc;
    }

    bool supports_streaming(TransportProtocol protocol) noexcept
    {
        return protocol == TransportProtocol::Grpc;
    }
}