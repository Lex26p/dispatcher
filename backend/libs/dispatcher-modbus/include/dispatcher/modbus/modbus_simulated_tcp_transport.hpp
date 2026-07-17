#pragma once

#include <dispatcher/modbus/modbus_tcp_transport.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace dispatcher::modbus
{
    class ModbusSimulatedTcpTransport final
        : public IModbusTcpTransport
    {
    public:
        ModbusSimulatedTcpTransport() = default;

        [[nodiscard]] ModbusBytes exchange(
            const ModbusBytes& request_frame
        ) override;

        void set_holding_register(
            std::uint8_t unit_id,
            std::uint16_t address,
            std::uint16_t value
        );

        void set_input_register(
            std::uint8_t unit_id,
            std::uint16_t address,
            std::uint16_t value
        );

        void set_holding_registers(
            std::uint8_t unit_id,
            std::uint16_t start_address,
            const std::vector<std::uint16_t>& values
        );

        void set_input_registers(
            std::uint8_t unit_id,
            std::uint16_t start_address,
            const std::vector<std::uint16_t>& values
        );

        void clear();

        [[nodiscard]] int exchange_count() const noexcept;

        [[nodiscard]] const std::vector<ModbusBytes>& requests() const noexcept;

    private:
        struct RegisterKey
        {
            std::uint8_t unit_id{ 0 };
            std::uint16_t address{ 0 };
        };

        struct RegisterKeyLess
        {
            [[nodiscard]] bool operator()(
                const RegisterKey& left,
                const RegisterKey& right
                ) const noexcept
            {
                if (left.unit_id != right.unit_id)
                {
                    return left.unit_id < right.unit_id;
                }

                return left.address < right.address;
            }
        };

        using RegisterMap =
            std::map<RegisterKey, std::uint16_t, RegisterKeyLess>;

        RegisterMap holding_registers_{};
        RegisterMap input_registers_{};

        int exchange_count_{ 0 };
        std::vector<ModbusBytes> requests_{};

        [[nodiscard]] ModbusBytes handle_read_registers(
            const ModbusBytes& request_frame,
            const RegisterMap& registers
        ) const;

        [[nodiscard]] std::optional<std::uint16_t> find_register(
            const RegisterMap& registers,
            std::uint8_t unit_id,
            std::uint16_t address
        ) const;

        static void set_registers(
            RegisterMap& registers,
            std::uint8_t unit_id,
            std::uint16_t start_address,
            const std::vector<std::uint16_t>& values
        );

        [[nodiscard]] static std::uint16_t read_u16_be(
            const ModbusBytes& bytes,
            std::size_t offset
        );

        static void append_u16_be(
            ModbusBytes& bytes,
            std::uint16_t value
        );

        [[nodiscard]] static ModbusBytes make_exception_response(
            std::uint16_t transaction_id,
            std::uint8_t unit_id,
            std::uint8_t function_code,
            std::uint8_t exception_code
        );
    };
}