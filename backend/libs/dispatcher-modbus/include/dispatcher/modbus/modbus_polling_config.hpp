#pragma once

#include <dispatcher/modbus/modbus_register_decoder.hpp>
#include <dispatcher/modbus/modbus_tcp_transport.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace dispatcher::modbus
{
    struct ModbusTagMapping
    {
        std::string tag_id{};

        std::uint8_t unit_id{ 1 };

        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };

        std::uint16_t address{ 0 };

        ModbusRegisterValueType value_type{
            ModbusRegisterValueType::uint16
        };

        ModbusWordOrder word_order{
            ModbusWordOrder::high_word_first
        };

        double scale{ 1.0 };
        double offset{ 0.0 };

        bool enabled{ true };
    };

    struct ModbusPollingConfiguration
    {
        ModbusTcpEndpoint endpoint{};

        std::uint32_t poll_interval_ms{ 1000 };
        std::uint32_t timeout_ms{ 1000 };
        std::uint32_t max_registers_per_request{ 125 };

        std::vector<ModbusTagMapping> mappings{};
    };

    struct ModbusPollRequest
    {
        std::uint8_t unit_id{ 1 };

        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };

        std::uint16_t start_address{ 0 };
        std::uint16_t quantity{ 1 };

        std::vector<ModbusTagMapping> mappings{};
    };

    class ModbusPollingPlanBuilder final
    {
    public:
        [[nodiscard]] static std::uint16_t required_register_count(
            ModbusRegisterValueType value_type
        );

        [[nodiscard]] static std::uint16_t end_address(
            const ModbusTagMapping& mapping
        );

        static void validate_mapping(
            const ModbusTagMapping& mapping,
            std::uint32_t max_registers_per_request
        );

        static void validate_configuration(
            const ModbusPollingConfiguration& configuration
        );

        [[nodiscard]] static std::vector<ModbusPollRequest> build_read_plan(
            const ModbusPollingConfiguration& configuration
        );

    private:
        [[nodiscard]] static bool can_merge(
            const ModbusPollRequest& request,
            const ModbusTagMapping& mapping,
            std::uint32_t max_registers_per_request
        );

        static void merge_mapping(
            ModbusPollRequest& request,
            const ModbusTagMapping& mapping
        );
    };
}