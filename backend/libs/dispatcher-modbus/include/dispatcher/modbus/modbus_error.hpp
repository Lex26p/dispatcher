#pragma once

#include <stdexcept>
#include <string>

namespace dispatcher::modbus
{
    class ModbusError final : public std::runtime_error
    {
    public:
        explicit ModbusError(
            const std::string& message
        )
            : std::runtime_error(
                message
            )
        {
        }
    };
}