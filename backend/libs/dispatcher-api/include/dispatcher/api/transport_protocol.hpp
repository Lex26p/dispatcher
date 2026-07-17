#pragma once

namespace dispatcher::api
{
    enum class TransportProtocol
    {
        Unknown,
        Http,
        Grpc
    };

    [[nodiscard]] const char* to_string(
        TransportProtocol protocol
    ) noexcept;

    [[nodiscard]] bool is_known_protocol(
        TransportProtocol protocol
    ) noexcept;

    [[nodiscard]] bool is_http_protocol(
        TransportProtocol protocol
    ) noexcept;

    [[nodiscard]] bool is_grpc_protocol(
        TransportProtocol protocol
    ) noexcept;

    [[nodiscard]] bool supports_streaming(
        TransportProtocol protocol
    ) noexcept;
}