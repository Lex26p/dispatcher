#include <dispatcher/modbus/modbus_tcp_client.hpp>

#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_frame_codec.hpp>

#include <string>

namespace dispatcher::modbus
{
    namespace
    {
        [[nodiscard]] std::string to_number_string(
            std::uint8_t value
        )
        {
            return std::to_string(
                static_cast<unsigned int>(
                    value
                    )
            );
        }

        [[nodiscard]] std::string to_number_string(
            std::uint16_t value
        )
        {
            return std::to_string(
                static_cast<unsigned int>(
                    value
                    )
            );
        }
    }

    ModbusTcpClient::ModbusTcpClient(
        IModbusTcpTransport& transport
    )
        : transport_(
            &transport
        )
    {
    }

    ModbusReadRegistersResponse ModbusTcpClient::read_holding_registers(
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity
    )
    {
        return read_registers(
            ModbusFunctionCode::read_holding_registers,
            unit_id,
            start_address,
            quantity
        );
    }

    ModbusReadRegistersResponse ModbusTcpClient::read_input_registers(
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity
    )
    {
        return read_registers(
            ModbusFunctionCode::read_input_registers,
            unit_id,
            start_address,
            quantity
        );
    }

    std::uint16_t ModbusTcpClient::next_transaction_id() const noexcept
    {
        return next_transaction_id_;
    }

    std::uint16_t ModbusTcpClient::allocate_transaction_id()
    {
        auto current =
            next_transaction_id_;

        if (current == 0)
        {
            current = 1;
        }

        if (current == 0xFFFFU)
        {
            next_transaction_id_ = 1;
        }
        else
        {
            next_transaction_id_ =
                static_cast<std::uint16_t>(
                    current + 1
                    );
        }

        return current;
    }

    ModbusReadRegistersResponse ModbusTcpClient::read_registers(
        ModbusFunctionCode function_code,
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity
    )
    {
        const ModbusReadRegistersRequest request{
            allocate_transaction_id(),
            unit_id,
            function_code,
            start_address,
            quantity
        };

        const auto request_frame =
            ModbusFrameCodec::encode_read_registers_request(
                request
            );

        const auto response_frame =
            transport_->exchange(
                request_frame
            );

        if (ModbusFrameCodec::is_exception_response(
            response_frame
        ))
        {
            const auto exception =
                ModbusFrameCodec::decode_exception_response(
                    response_frame
                );

            if (exception.transaction_id != request.transaction_id)
            {
                throw ModbusError(
                    "Modbus exception transaction id mismatch."
                );
            }

            if (exception.unit_id != request.unit_id)
            {
                throw ModbusError(
                    "Modbus exception unit id mismatch."
                );
            }

            throw ModbusError(
                "Modbus exception response. Function="
                + to_number_string(
                    static_cast<std::uint8_t>(
                        exception.function_code
                        )
                )
                + ", exception_code="
                + to_number_string(
                    exception.exception_code
                )
                + "."
            );
        }

        auto response =
            ModbusFrameCodec::decode_read_registers_response(
                response_frame
            );

        validate_response(
            request,
            response
        );

        return response;
    }

    void ModbusTcpClient::validate_response(
        const ModbusReadRegistersRequest& request,
        const ModbusReadRegistersResponse& response
    )
    {
        if (response.transaction_id != request.transaction_id)
        {
            throw ModbusError(
                "Modbus response transaction id mismatch."
            );
        }

        if (response.unit_id != request.unit_id)
        {
            throw ModbusError(
                "Modbus response unit id mismatch."
            );
        }

        if (response.function_code != request.function_code)
        {
            throw ModbusError(
                "Modbus response function code mismatch."
            );
        }

        if (response.registers.size() != request.quantity)
        {
            throw ModbusError(
                "Modbus response register count mismatch."
            );
        }
    }
}