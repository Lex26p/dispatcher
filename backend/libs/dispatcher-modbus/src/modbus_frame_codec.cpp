#include <dispatcher/modbus/modbus_frame_codec.hpp>

#include <dispatcher/modbus/modbus_error.hpp>

#include <cstddef>
#include <string>

namespace dispatcher::modbus
{
    namespace
    {
        constexpr std::uint16_t modbus_tcp_protocol_id = 0;
        constexpr std::size_t mbap_header_size = 7;
        constexpr std::uint16_t max_read_register_quantity = 125;

        [[nodiscard]] std::uint8_t to_u8(
            ModbusFunctionCode function_code
        )
        {
            return static_cast<std::uint8_t>(
                function_code
                );
        }

        [[nodiscard]] ModbusFunctionCode to_function_code(
            std::uint8_t value
        )
        {
            return static_cast<ModbusFunctionCode>(
                value
                );
        }
    }

    ModbusBytes ModbusFrameCodec::encode_read_registers_request(
        const ModbusReadRegistersRequest& request
    )
    {
        if (!is_read_registers_function(
            request.function_code
        ))
        {
            throw ModbusError(
                "Unsupported Modbus read registers function code."
            );
        }

        if (request.unit_id == 0)
        {
            throw ModbusError(
                "Modbus unit id must not be zero."
            );
        }

        if (request.quantity == 0)
        {
            throw ModbusError(
                "Modbus register quantity must be greater than zero."
            );
        }

        if (request.quantity > max_read_register_quantity)
        {
            throw ModbusError(
                "Modbus register quantity exceeds maximum read size."
            );
        }

        ModbusBytes frame;
        frame.reserve(
            12
        );

        append_u16_be(
            frame,
            request.transaction_id
        );

        append_u16_be(
            frame,
            modbus_tcp_protocol_id
        );

        append_u16_be(
            frame,
            6
        );

        frame.push_back(
            request.unit_id
        );

        frame.push_back(
            to_u8(
                request.function_code
            )
        );

        append_u16_be(
            frame,
            request.start_address
        );

        append_u16_be(
            frame,
            request.quantity
        );

        return frame;
    }

    ModbusReadRegistersResponse ModbusFrameCodec::decode_read_registers_response(
        const ModbusBytes& frame
    )
    {
        if (frame.size() < mbap_header_size + 2)
        {
            throw ModbusError(
                "Modbus response frame is too short."
            );
        }

        if (is_exception_response(
            frame
        ))
        {
            throw ModbusError(
                "Modbus response contains exception."
            );
        }

        const auto protocol_id =
            read_u16_be(
                frame,
                2
            );

        if (protocol_id != modbus_tcp_protocol_id)
        {
            throw ModbusError(
                "Invalid Modbus TCP protocol id."
            );
        }

        const auto length =
            read_u16_be(
                frame,
                4
            );

        if (length < 3)
        {
            throw ModbusError(
                "Invalid Modbus TCP response length."
            );
        }

        if (frame.size() != mbap_header_size + static_cast<std::size_t>(length - 1))
        {
            throw ModbusError(
                "Modbus TCP response length does not match frame size."
            );
        }

        const auto function_code =
            to_function_code(
                frame[7]
            );

        if (!is_read_registers_function(
            function_code
        ))
        {
            throw ModbusError(
                "Unsupported Modbus read registers response function code."
            );
        }

        const auto byte_count =
            frame[8];

        if (byte_count == 0)
        {
            throw ModbusError(
                "Modbus read registers response byte count must not be zero."
            );
        }

        if ((byte_count % 2) != 0)
        {
            throw ModbusError(
                "Modbus read registers response byte count must be even."
            );
        }

        if (frame.size() != mbap_header_size + 2 + byte_count)
        {
            throw ModbusError(
                "Modbus read registers response byte count does not match frame size."
            );
        }

        ModbusReadRegistersResponse response;

        response.transaction_id =
            read_u16_be(
                frame,
                0
            );

        response.unit_id =
            frame[6];

        response.function_code =
            function_code;

        for (std::size_t offset = 9; offset < frame.size(); offset += 2)
        {
            response.registers.push_back(
                read_u16_be(
                    frame,
                    offset
                )
            );
        }

        return response;
    }

    bool ModbusFrameCodec::is_exception_response(
        const ModbusBytes& frame
    )
    {
        if (frame.size() < mbap_header_size + 2)
        {
            return false;
        }

        const auto function_code =
            frame[7];

        return (function_code & 0x80U) != 0U;
    }

    ModbusExceptionResponse ModbusFrameCodec::decode_exception_response(
        const ModbusBytes& frame
    )
    {
        if (frame.size() < mbap_header_size + 2)
        {
            throw ModbusError(
                "Modbus exception response frame is too short."
            );
        }

        if (!is_exception_response(
            frame
        ))
        {
            throw ModbusError(
                "Modbus frame is not an exception response."
            );
        }

        const auto protocol_id =
            read_u16_be(
                frame,
                2
            );

        if (protocol_id != modbus_tcp_protocol_id)
        {
            throw ModbusError(
                "Invalid Modbus TCP protocol id."
            );
        }

        const auto length =
            read_u16_be(
                frame,
                4
            );

        if (length != 3)
        {
            throw ModbusError(
                "Invalid Modbus exception response length."
            );
        }

        if (frame.size() != mbap_header_size + 2)
        {
            throw ModbusError(
                "Modbus exception response frame size mismatch."
            );
        }

        const auto exception_function =
            frame[7];

        ModbusExceptionResponse response;

        response.transaction_id =
            read_u16_be(
                frame,
                0
            );

        response.unit_id =
            frame[6];

        response.function_code =
            to_function_code(
                static_cast<std::uint8_t>(
                    exception_function & 0x7FU
                    )
            );

        response.exception_code =
            frame[8];

        return response;
    }

    bool ModbusFrameCodec::is_read_registers_function(
        ModbusFunctionCode function_code
    )
    {
        return function_code == ModbusFunctionCode::read_holding_registers
            || function_code == ModbusFunctionCode::read_input_registers;
    }

    std::uint16_t ModbusFrameCodec::read_u16_be(
        const ModbusBytes& bytes,
        std::size_t offset
    )
    {
        if (offset + 1 >= bytes.size())
        {
            throw ModbusError(
                "Cannot read uint16 from Modbus frame."
            );
        }

        return static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                bytes[offset]
                )
            << 8U
            | static_cast<std::uint16_t>(
                bytes[offset + 1]
                )
            );
    }

    void ModbusFrameCodec::append_u16_be(
        ModbusBytes& bytes,
        std::uint16_t value
    )
    {
        bytes.push_back(
            static_cast<std::uint8_t>(
                (value >> 8U) & 0xFFU
                )
        );

        bytes.push_back(
            static_cast<std::uint8_t>(
                value & 0xFFU
                )
        );
    }
}