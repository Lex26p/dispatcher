#include <dispatcher/modbus/modbus_polling_config.hpp>

#include <dispatcher/modbus/modbus_error.hpp>

#include <algorithm>
#include <set>
#include <string>

namespace dispatcher::modbus
{
    namespace
    {
        constexpr std::uint32_t max_modbus_register_address = 65535;
        constexpr std::uint32_t max_modbus_registers_per_read = 125;

        [[nodiscard]] std::uint32_t mapping_start(
            const ModbusTagMapping& mapping
        )
        {
            return static_cast<std::uint32_t>(
                mapping.address
                );
        }

        [[nodiscard]] std::uint32_t mapping_end(
            const ModbusTagMapping& mapping
        )
        {
            return mapping_start(
                mapping
            )
                + static_cast<std::uint32_t>(
                    ModbusPollingPlanBuilder::required_register_count(
                        mapping.value_type
                    )
                    )
                - 1U;
        }

        [[nodiscard]] std::uint32_t request_end(
            const ModbusPollRequest& request
        )
        {
            return static_cast<std::uint32_t>(
                request.start_address
                )
                + static_cast<std::uint32_t>(
                    request.quantity
                    )
                - 1U;
        }

        [[nodiscard]] std::uint8_t function_code_to_u8(
            ModbusFunctionCode function_code
        )
        {
            return static_cast<std::uint8_t>(
                function_code
                );
        }
    }

    std::uint16_t ModbusPollingPlanBuilder::required_register_count(
        ModbusRegisterValueType value_type
    )
    {
        switch (value_type)
        {
        case ModbusRegisterValueType::uint16:
        case ModbusRegisterValueType::int16:
            return 1;

        case ModbusRegisterValueType::uint32:
        case ModbusRegisterValueType::int32:
        case ModbusRegisterValueType::float32:
            return 2;
        }

        return 1;
    }

    std::uint16_t ModbusPollingPlanBuilder::end_address(
        const ModbusTagMapping& mapping
    )
    {
        validate_mapping(
            mapping,
            max_modbus_registers_per_read
        );

        return static_cast<std::uint16_t>(
            mapping_end(
                mapping
            )
            );
    }

    void ModbusPollingPlanBuilder::validate_mapping(
        const ModbusTagMapping& mapping,
        std::uint32_t max_registers_per_request
    )
    {
        if (mapping.tag_id.empty())
        {
            throw ModbusError(
                "Modbus tag mapping tag_id must not be empty."
            );
        }

        if (mapping.unit_id == 0)
        {
            throw ModbusError(
                "Modbus tag mapping unit_id must not be zero."
            );
        }

        if (max_registers_per_request == 0)
        {
            throw ModbusError(
                "Modbus max_registers_per_request must be greater than zero."
            );
        }

        if (max_registers_per_request > max_modbus_registers_per_read)
        {
            throw ModbusError(
                "Modbus max_registers_per_request exceeds protocol maximum."
            );
        }

        const auto required_registers =
            required_register_count(
                mapping.value_type
            );

        if (required_registers > max_registers_per_request)
        {
            throw ModbusError(
                "Modbus tag mapping requires more registers than max request size."
            );
        }

        const auto last_address =
            mapping_end(
                mapping
            );

        if (last_address > max_modbus_register_address)
        {
            throw ModbusError(
                "Modbus tag mapping register range exceeds address space."
            );
        }

        if (mapping.function_code != ModbusFunctionCode::read_holding_registers
            && mapping.function_code != ModbusFunctionCode::read_input_registers)
        {
            throw ModbusError(
                "Modbus tag mapping function_code must read registers."
            );
        }
    }

    void ModbusPollingPlanBuilder::validate_configuration(
        const ModbusPollingConfiguration& configuration
    )
    {
        if (configuration.endpoint.host.empty())
        {
            throw ModbusError(
                "Modbus endpoint host must not be empty."
            );
        }

        if (configuration.endpoint.port == 0)
        {
            throw ModbusError(
                "Modbus endpoint port must not be zero."
            );
        }

        if (configuration.poll_interval_ms == 0)
        {
            throw ModbusError(
                "Modbus poll_interval_ms must not be zero."
            );
        }

        if (configuration.timeout_ms == 0)
        {
            throw ModbusError(
                "Modbus timeout_ms must not be zero."
            );
        }

        if (configuration.max_registers_per_request == 0)
        {
            throw ModbusError(
                "Modbus max_registers_per_request must not be zero."
            );
        }

        if (configuration.max_registers_per_request > max_modbus_registers_per_read)
        {
            throw ModbusError(
                "Modbus max_registers_per_request exceeds protocol maximum."
            );
        }

        std::set<std::string> tag_ids;

        for (const auto& mapping : configuration.mappings)
        {
            if (!mapping.enabled)
            {
                continue;
            }

            validate_mapping(
                mapping,
                configuration.max_registers_per_request
            );

            const auto inserted =
                tag_ids.insert(
                    mapping.tag_id
                );

            if (!inserted.second)
            {
                throw ModbusError(
                    "Duplicate enabled Modbus tag mapping: " + mapping.tag_id
                );
            }
        }
    }

    std::vector<ModbusPollRequest> ModbusPollingPlanBuilder::build_read_plan(
        const ModbusPollingConfiguration& configuration
    )
    {
        validate_configuration(
            configuration
        );

        std::vector<ModbusTagMapping> enabled_mappings;

        for (const auto& mapping : configuration.mappings)
        {
            if (mapping.enabled)
            {
                enabled_mappings.push_back(
                    mapping
                );
            }
        }

        std::sort(
            enabled_mappings.begin(),
            enabled_mappings.end(),
            [](const ModbusTagMapping& left, const ModbusTagMapping& right)
            {
                if (left.unit_id != right.unit_id)
                {
                    return left.unit_id < right.unit_id;
                }

                if (left.function_code != right.function_code)
                {
                    return function_code_to_u8(
                        left.function_code
                    )
                        < function_code_to_u8(
                            right.function_code
                        );
                }

                if (left.address != right.address)
                {
                    return left.address < right.address;
                }

                return left.tag_id < right.tag_id;
            }
        );

        std::vector<ModbusPollRequest> plan;

        for (const auto& mapping : enabled_mappings)
        {
            if (!plan.empty()
                && can_merge(
                    plan.back(),
                    mapping,
                    configuration.max_registers_per_request
                ))
            {
                merge_mapping(
                    plan.back(),
                    mapping
                );

                continue;
            }

            ModbusPollRequest request;

            request.unit_id =
                mapping.unit_id;

            request.function_code =
                mapping.function_code;

            request.start_address =
                mapping.address;

            request.quantity =
                required_register_count(
                    mapping.value_type
                );

            request.mappings.push_back(
                mapping
            );

            plan.push_back(
                std::move(
                    request
                )
            );
        }

        return plan;
    }

    bool ModbusPollingPlanBuilder::can_merge(
        const ModbusPollRequest& request,
        const ModbusTagMapping& mapping,
        std::uint32_t max_registers_per_request
    )
    {
        if (request.unit_id != mapping.unit_id)
        {
            return false;
        }

        if (request.function_code != mapping.function_code)
        {
            return false;
        }

        const auto start =
            static_cast<std::uint32_t>(
                request.start_address
                );

        const auto merged_end =
            std::max(
                request_end(
                    request
                ),
                mapping_end(
                    mapping
                )
            );

        const auto merged_quantity =
            merged_end - start + 1U;

        return merged_quantity <= max_registers_per_request;
    }

    void ModbusPollingPlanBuilder::merge_mapping(
        ModbusPollRequest& request,
        const ModbusTagMapping& mapping
    )
    {
        const auto merged_end =
            std::max(
                request_end(
                    request
                ),
                mapping_end(
                    mapping
                )
            );

        request.quantity =
            static_cast<std::uint16_t>(
                merged_end
                - static_cast<std::uint32_t>(
                    request.start_address
                    )
                + 1U
                );

        request.mappings.push_back(
            mapping
        );
    }
}