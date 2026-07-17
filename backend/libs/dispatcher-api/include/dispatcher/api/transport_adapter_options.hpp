#pragma once

#include <dispatcher/api/transport_protocol.hpp>

#include <chrono>
#include <cstdint>
#include <string>

namespace dispatcher::api
{
    class TransportAdapterOptions
    {
    public:
        using Timeout = std::chrono::milliseconds;

        TransportAdapterOptions(
            TransportProtocol protocol,
            std::string bind_address = "127.0.0.1",
            std::uint16_t port = 0,
            bool enabled = true,
            Timeout request_timeout = Timeout{ 30000 }
        );

        [[nodiscard]] TransportProtocol protocol() const noexcept;

        [[nodiscard]] const std::string& bind_address() const noexcept;

        [[nodiscard]] std::uint16_t port() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] bool disabled() const noexcept;

        [[nodiscard]] Timeout request_timeout() const noexcept;

        [[nodiscard]] bool has_bind_address() const noexcept;

        [[nodiscard]] bool has_port() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

    private:
        TransportProtocol protocol_{ TransportProtocol::Unknown };
        std::string bind_address_;
        std::uint16_t port_{ 0 };
        bool enabled_{ true };
        Timeout request_timeout_{ 30000 };
    };
}