#pragma once

#include <dispatcher/modbus/modbus_types.hpp>

#include <cstdint>

namespace dispatcher::modbus
{
    class ModbusFrameCodec final
    {
    public:
        [[nodiscard]] static ModbusBytes encode_read_registers_request(
            const ModbusReadRegistersRequest& request
        );

        [[nodiscard]] static ModbusReadRegistersResponse decode_read_registers_response(
            const ModbusBytes& frame
        );

        [[nodiscard]] static bool is_exception_response(
            const ModbusBytes& frame
        );

        [[nodiscard]] static ModbusExceptionResponse decode_exception_response(
            const ModbusBytes& frame
        );

    private:
        [[nodiscard]] static bool is_read_registers_function(
            ModbusFunctionCode function_code
        );

        [[nodiscard]] static std::uint16_t read_u16_be(
            const ModbusBytes& bytes,
            std::size_t offset
        );

        static void append_u16_be(
            ModbusBytes& bytes,
            std::uint16_t value
        );
    };
}