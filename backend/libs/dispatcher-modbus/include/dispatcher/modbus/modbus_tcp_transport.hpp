#pragma once

#include <dispatcher/modbus/modbus_types.hpp>

#include <cstdint>
#include <string>

namespace dispatcher::modbus
{
    struct ModbusTcpEndpoint
    {
        std::string host{ "127.0.0.1" };
        std::uint16_t port{ 502 };
    };

    class IModbusTcpTransport
    {
    public:
        virtual ~IModbusTcpTransport() = default;

        [[nodiscard]] virtual ModbusBytes exchange(
            const ModbusBytes& request_frame
        ) = 0;
    };
}