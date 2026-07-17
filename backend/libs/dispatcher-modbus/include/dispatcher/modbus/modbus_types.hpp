#pragma once

#include <cstdint>
#include <vector>

namespace dispatcher::modbus
{
    using ModbusBytes = std::vector<std::uint8_t>;

    enum class ModbusFunctionCode : std::uint8_t
    {
        read_coils = 0x01,
        read_discrete_inputs = 0x02,
        read_holding_registers = 0x03,
        read_input_registers = 0x04
    };

    struct ModbusReadRegistersRequest
    {
        std::uint16_t transaction_id{ 0 };
        std::uint8_t unit_id{ 1 };
        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };
        std::uint16_t start_address{ 0 };
        std::uint16_t quantity{ 1 };
    };

    struct ModbusReadRegistersResponse
    {
        std::uint16_t transaction_id{ 0 };
        std::uint8_t unit_id{ 0 };
        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };
        std::vector<std::uint16_t> registers{};
    };

    struct ModbusExceptionResponse
    {
        std::uint16_t transaction_id{ 0 };
        std::uint8_t unit_id{ 0 };
        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };
        std::uint8_t exception_code{ 0 };
    };
}