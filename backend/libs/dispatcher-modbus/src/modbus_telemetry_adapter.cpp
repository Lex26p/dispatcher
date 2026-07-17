#include <dispatcher/modbus/modbus_telemetry_adapter.hpp>

#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_register_decoder.hpp>

#include <cstddef>
#include <exception>
#include <string>
#include <utility>

namespace dispatcher::modbus
{
    namespace
    {
        [[nodiscard]] std::size_t mapping_offset(
            const ModbusPollRequest& request,
            const ModbusTagMapping& mapping
        )
        {
            if (mapping.address < request.start_address)
            {
                throw ModbusError(
                    "Modbus mapping address is before request start address."
                );
            }

            return static_cast<std::size_t>(
                mapping.address - request.start_address
                );
        }
    }

    ModbusTelemetryAdapter::ModbusTelemetryAdapter(
        ModbusPollingConfiguration configuration,
        IModbusTcpTransport& transport
    )
        : configuration_(
            std::move(
                configuration
            )
        )
        , read_plan_(
            ModbusPollingPlanBuilder::build_read_plan(
                configuration_
            )
        )
        , client_(
            transport
        )
    {
    }

    const ModbusPollingConfiguration& ModbusTelemetryAdapter::configuration() const noexcept
    {
        return configuration_;
    }

    const std::vector<ModbusPollRequest>& ModbusTelemetryAdapter::read_plan() const noexcept
    {
        return read_plan_;
    }

    ModbusTelemetryPollResult ModbusTelemetryAdapter::poll_once()
    {
        ModbusTelemetryPollResult result;

        for (const auto& request : read_plan_)
        {
            try
            {
                const auto response =
                    execute_request(
                        request
                    );

                for (const auto& mapping : request.mappings)
                {
                    result.samples.push_back(
                        decode_mapping(
                            request,
                            mapping,
                            response
                        )
                    );
                }
            }
            catch (const std::exception& exception)
            {
                ModbusTelemetryPollError error;

                error.unit_id =
                    request.unit_id;

                error.function_code =
                    request.function_code;

                error.start_address =
                    request.start_address;

                error.quantity =
                    request.quantity;

                error.message =
                    exception.what();

                result.errors.push_back(
                    std::move(
                        error
                    )
                );
            }
        }

        return result;
    }

    ModbusReadRegistersResponse ModbusTelemetryAdapter::execute_request(
        const ModbusPollRequest& request
    )
    {
        switch (request.function_code)
        {
        case ModbusFunctionCode::read_holding_registers:
            return client_.read_holding_registers(
                request.unit_id,
                request.start_address,
                request.quantity
            );

        case ModbusFunctionCode::read_input_registers:
            return client_.read_input_registers(
                request.unit_id,
                request.start_address,
                request.quantity
            );

        case ModbusFunctionCode::read_coils:
        case ModbusFunctionCode::read_discrete_inputs:
            break;
        }

        throw ModbusError(
            "Unsupported Modbus telemetry request function code."
        );
    }

    ModbusTelemetrySample ModbusTelemetryAdapter::decode_mapping(
        const ModbusPollRequest& request,
        const ModbusTagMapping& mapping,
        const ModbusReadRegistersResponse& response
    )
    {
        const auto offset =
            mapping_offset(
                request,
                mapping
            );

        const auto decoded =
            ModbusRegisterDecoder::decode_numeric(
                response.registers,
                mapping.value_type,
                offset,
                mapping.word_order
            );

        ModbusTelemetrySample sample;

        sample.tag_id =
            mapping.tag_id;

        sample.value =
            apply_linear_transform(
                decoded.numeric_value,
                mapping.scale,
                mapping.offset
            );

        sample.quality =
            "good";

        sample.source =
            "modbus-tcp";

        sample.unit_id =
            mapping.unit_id;

        sample.function_code =
            mapping.function_code;

        sample.address =
            mapping.address;

        sample.value_type =
            mapping.value_type;

        return sample;
    }

    double ModbusTelemetryAdapter::apply_linear_transform(
        double raw_value,
        double scale,
        double offset
    )
    {
        return raw_value * scale + offset;
    }
}