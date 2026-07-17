#include <dispatcher/modbus/modbus_simulated_tcp_transport.hpp>

#include <dispatcher/modbus/modbus_error.hpp>

#include <cstddef>
#include <string>

namespace dispatcher::modbus
{
    namespace
    {
        constexpr std::uint16_t modbus_tcp_protocol_id = 0;
        constexpr std::size_t read_registers_request_size = 12;
        constexpr std::uint16_t max_read_register_quantity = 125;

        constexpr std::uint8_t exception_illegal_function = 0x01;
        constexpr std::uint8_t exception_illegal_data_address = 0x02;
        constexpr std::uint8_t exception_illegal_data_value = 0x03;
    }

    ModbusBytes ModbusSimulatedTcpTransport::exchange(
        const ModbusBytes& request_frame
    )
    {
        ++exchange_count_;

        requests_.push_back(
            request_frame
        );

        if (request_frame.size() != read_registers_request_size)
        {
            throw ModbusError(
                "Simulated Modbus transport received malformed request frame."
            );
        }

        const auto protocol_id =
            read_u16_be(
                request_frame,
                2
            );

        if (protocol_id != modbus_tcp_protocol_id)
        {
            throw ModbusError(
                "Simulated Modbus transport received invalid protocol id."
            );
        }

        const auto length =
            read_u16_be(
                request_frame,
                4
            );

        if (length != 6)
        {
            throw ModbusError(
                "Simulated Modbus transport received invalid request length."
            );
        }

        const auto function_code =
            request_frame[7];

        if (function_code == static_cast<std::uint8_t>(
            ModbusFunctionCode::read_holding_registers
            ))
        {
            return handle_read_registers(
                request_frame,
                holding_registers_
            );
        }

        if (function_code == static_cast<std::uint8_t>(
            ModbusFunctionCode::read_input_registers
            ))
        {
            return handle_read_registers(
                request_frame,
                input_registers_
            );
        }

        return make_exception_response(
            read_u16_be(
                request_frame,
                0
            ),
            request_frame[6],
            function_code,
            exception_illegal_function
        );
    }

    void ModbusSimulatedTcpTransport::set_holding_register(
        std::uint8_t unit_id,
        std::uint16_t address,
        std::uint16_t value
    )
    {
        holding_registers_[RegisterKey{ unit_id, address }] =
            value;
    }

    void ModbusSimulatedTcpTransport::set_input_register(
        std::uint8_t unit_id,
        std::uint16_t address,
        std::uint16_t value
    )
    {
        input_registers_[RegisterKey{ unit_id, address }] =
            value;
    }

    void ModbusSimulatedTcpTransport::set_holding_registers(
        std::uint8_t unit_id,
        std::uint16_t start_address,
        const std::vector<std::uint16_t>& values
    )
    {
        set_registers(
            holding_registers_,
            unit_id,
            start_address,
            values
        );
    }

    void ModbusSimulatedTcpTransport::set_input_registers(
        std::uint8_t unit_id,
        std::uint16_t start_address,
        const std::vector<std::uint16_t>& values
    )
    {
        set_registers(
            input_registers_,
            unit_id,
            start_address,
            values
        );
    }

    void ModbusSimulatedTcpTransport::clear()
    {
        holding_registers_.clear();
        input_registers_.clear();
        requests_.clear();
        exchange_count_ = 0;
    }

    int ModbusSimulatedTcpTransport::exchange_count() const noexcept
    {
        return exchange_count_;
    }

    const std::vector<ModbusBytes>& ModbusSimulatedTcpTransport::requests() const noexcept
    {
        return requests_;
    }

    ModbusBytes ModbusSimulatedTcpTransport::handle_read_registers(
        const ModbusBytes& request_frame,
        const RegisterMap& registers
    ) const
    {
        const auto transaction_id =
            read_u16_be(
                request_frame,
                0
            );

        const auto unit_id =
            request_frame[6];

        const auto function_code =
            request_frame[7];

        const auto start_address =
            read_u16_be(
                request_frame,
                8
            );

        const auto quantity =
            read_u16_be(
                request_frame,
                10
            );

        if (unit_id == 0)
        {
            return make_exception_response(
                transaction_id,
                unit_id,
                function_code,
                exception_illegal_data_value
            );
        }

        if (quantity == 0 || quantity > max_read_register_quantity)
        {
            return make_exception_response(
                transaction_id,
                unit_id,
                function_code,
                exception_illegal_data_value
            );
        }

        std::vector<std::uint16_t> values;
        values.reserve(
            quantity
        );

        for (std::uint16_t index = 0; index < quantity; ++index)
        {
            const auto address =
                static_cast<std::uint16_t>(
                    start_address + index
                    );

            const auto value =
                find_register(
                    registers,
                    unit_id,
                    address
                );

            if (!value.has_value())
            {
                return make_exception_response(
                    transaction_id,
                    unit_id,
                    function_code,
                    exception_illegal_data_address
                );
            }

            values.push_back(
                *value
            );
        }

        const auto byte_count =
            static_cast<std::uint8_t>(
                values.size() * 2U
                );

        const auto length =
            static_cast<std::uint16_t>(
                3U + byte_count
                );

        ModbusBytes response;
        response.reserve(
            9U + byte_count
        );

        append_u16_be(
            response,
            transaction_id
        );

        append_u16_be(
            response,
            modbus_tcp_protocol_id
        );

        append_u16_be(
            response,
            length
        );

        response.push_back(
            unit_id
        );

        response.push_back(
            function_code
        );

        response.push_back(
            byte_count
        );

        for (const auto value : values)
        {
            append_u16_be(
                response,
                value
            );
        }

        return response;
    }

    std::optional<std::uint16_t> ModbusSimulatedTcpTransport::find_register(
        const RegisterMap& registers,
        std::uint8_t unit_id,
        std::uint16_t address
    ) const
    {
        const auto iterator =
            registers.find(
                RegisterKey{
                    unit_id,
                    address
                }
            );

        if (iterator == registers.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    void ModbusSimulatedTcpTransport::set_registers(
        RegisterMap& registers,
        std::uint8_t unit_id,
        std::uint16_t start_address,
        const std::vector<std::uint16_t>& values
    )
    {
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            const auto address =
                static_cast<std::uint16_t>(
                    start_address
                    + static_cast<std::uint16_t>(
                        index
                        )
                    );

            registers[RegisterKey{ unit_id, address }] =
                values[index];
        }
    }

    std::uint16_t ModbusSimulatedTcpTransport::read_u16_be(
        const ModbusBytes& bytes,
        std::size_t offset
    )
    {
        if (offset + 1 >= bytes.size())
        {
            throw ModbusError(
                "Simulated Modbus transport cannot read uint16."
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

    void ModbusSimulatedTcpTransport::append_u16_be(
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

    ModbusBytes ModbusSimulatedTcpTransport::make_exception_response(
        std::uint16_t transaction_id,
        std::uint8_t unit_id,
        std::uint8_t function_code,
        std::uint8_t exception_code
    )
    {
        ModbusBytes response;

        append_u16_be(
            response,
            transaction_id
        );

        append_u16_be(
            response,
            modbus_tcp_protocol_id
        );

        append_u16_be(
            response,
            3
        );

        response.push_back(
            unit_id
        );

        response.push_back(
            static_cast<std::uint8_t>(
                function_code | 0x80U
                )
        );

        response.push_back(
            exception_code
        );

        return response;
    }
}