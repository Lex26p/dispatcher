#pragma once

#include <dispatcher/modbus/modbus_tcp_transport.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <cstdint>

namespace dispatcher::modbus
{
    class ModbusTcpClient final
    {
    public:
        explicit ModbusTcpClient(
            IModbusTcpTransport& transport
        );

        [[nodiscard]] ModbusReadRegistersResponse read_holding_registers(
            std::uint8_t unit_id,
            std::uint16_t start_address,
            std::uint16_t quantity
        );

        [[nodiscard]] ModbusReadRegistersResponse read_input_registers(
            std::uint8_t unit_id,
            std::uint16_t start_address,
            std::uint16_t quantity
        );

        [[nodiscard]] std::uint16_t next_transaction_id() const noexcept;

    private:
        IModbusTcpTransport* transport_;
        std::uint16_t next_transaction_id_{ 1 };

        [[nodiscard]] std::uint16_t allocate_transaction_id();

        [[nodiscard]] ModbusReadRegistersResponse read_registers(
            ModbusFunctionCode function_code,
            std::uint8_t unit_id,
            std::uint16_t start_address,
            std::uint16_t quantity
        );

        static void validate_response(
            const ModbusReadRegistersRequest& request,
            const ModbusReadRegistersResponse& response
        );
    };
}