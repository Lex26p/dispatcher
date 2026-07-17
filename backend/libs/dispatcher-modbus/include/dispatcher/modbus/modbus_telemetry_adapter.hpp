#pragma once

#include <dispatcher/modbus/modbus_polling_config.hpp>
#include <dispatcher/modbus/modbus_tcp_client.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace dispatcher::modbus
{
    struct ModbusTelemetrySample
    {
        std::string tag_id{};

        double value{ 0.0 };

        std::string quality{ "good" };
        std::string source{ "modbus-tcp" };

        std::uint8_t unit_id{ 1 };

        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };

        std::uint16_t address{ 0 };

        ModbusRegisterValueType value_type{
            ModbusRegisterValueType::uint16
        };
    };

    struct ModbusTelemetryPollError
    {
        std::uint8_t unit_id{ 1 };

        ModbusFunctionCode function_code{
            ModbusFunctionCode::read_holding_registers
        };

        std::uint16_t start_address{ 0 };
        std::uint16_t quantity{ 0 };

        std::string message{};
    };

    struct ModbusTelemetryPollResult
    {
        std::vector<ModbusTelemetrySample> samples{};
        std::vector<ModbusTelemetryPollError> errors{};

        [[nodiscard]] bool success() const noexcept
        {
            return errors.empty();
        }

        [[nodiscard]] bool has_samples() const noexcept
        {
            return !samples.empty();
        }
    };

    class ModbusTelemetryAdapter final
    {
    public:
        ModbusTelemetryAdapter(
            ModbusPollingConfiguration configuration,
            IModbusTcpTransport& transport
        );

        [[nodiscard]] const ModbusPollingConfiguration& configuration() const noexcept;

        [[nodiscard]] const std::vector<ModbusPollRequest>& read_plan() const noexcept;

        [[nodiscard]] ModbusTelemetryPollResult poll_once();

    private:
        ModbusPollingConfiguration configuration_;
        std::vector<ModbusPollRequest> read_plan_;
        ModbusTcpClient client_;

        [[nodiscard]] ModbusReadRegistersResponse execute_request(
            const ModbusPollRequest& request
        );

        [[nodiscard]] static ModbusTelemetrySample decode_mapping(
            const ModbusPollRequest& request,
            const ModbusTagMapping& mapping,
            const ModbusReadRegistersResponse& response
        );

        [[nodiscard]] static double apply_linear_transform(
            double raw_value,
            double scale,
            double offset
        );
    };
}