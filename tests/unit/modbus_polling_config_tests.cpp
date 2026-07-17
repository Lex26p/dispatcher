#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_polling_config.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{
    dispatcher::modbus::ModbusTagMapping make_mapping(
        std::string tag_id,
        std::uint16_t address,
        dispatcher::modbus::ModbusRegisterValueType value_type =
        dispatcher::modbus::ModbusRegisterValueType::uint16
    )
    {
        dispatcher::modbus::ModbusTagMapping mapping;

        mapping.tag_id =
            std::move(
                tag_id
            );

        mapping.unit_id = 1;
        mapping.function_code =
            dispatcher::modbus::ModbusFunctionCode::read_holding_registers;
        mapping.address =
            address;
        mapping.value_type =
            value_type;

        return mapping;
    }

    dispatcher::modbus::ModbusPollingConfiguration make_configuration()
    {
        dispatcher::modbus::ModbusPollingConfiguration configuration;

        configuration.endpoint.host = "127.0.0.1";
        configuration.endpoint.port = 502;
        configuration.poll_interval_ms = 1000;
        configuration.timeout_ms = 500;
        configuration.max_registers_per_request = 125;

        return configuration;
    }

    void expect_validate_configuration_throws(
        const dispatcher::modbus::ModbusPollingConfiguration& configuration
    )
    {
        EXPECT_THROW(
            dispatcher::modbus::ModbusPollingPlanBuilder::validate_configuration(
                configuration
            ),
            dispatcher::modbus::ModbusError
        );
    }

    void expect_build_plan_throws(
        const dispatcher::modbus::ModbusPollingConfiguration& configuration
    )
    {
        EXPECT_THROW(
            {
                const auto plan =
                    dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
                        configuration
                    );

                static_cast<void>(
                    plan
                );
            },
            dispatcher::modbus::ModbusError
        );
    }
}

TEST(ModbusPollingConfigTests, RequiredRegisterCountForValueTypes)
{
    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::required_register_count(
            dispatcher::modbus::ModbusRegisterValueType::uint16
        ),
        1
    );

    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::required_register_count(
            dispatcher::modbus::ModbusRegisterValueType::int16
        ),
        1
    );

    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::required_register_count(
            dispatcher::modbus::ModbusRegisterValueType::uint32
        ),
        2
    );

    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::required_register_count(
            dispatcher::modbus::ModbusRegisterValueType::int32
        ),
        2
    );

    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::required_register_count(
            dispatcher::modbus::ModbusRegisterValueType::float32
        ),
        2
    );
}

TEST(ModbusPollingConfigTests, ComputesEndAddress)
{
    const auto mapping =
        make_mapping(
            "pump.pressure",
            10,
            dispatcher::modbus::ModbusRegisterValueType::float32
        );

    EXPECT_EQ(
        dispatcher::modbus::ModbusPollingPlanBuilder::end_address(
            mapping
        ),
        11
    );
}

TEST(ModbusPollingConfigTests, ValidatesConfiguration)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    EXPECT_NO_THROW(
        dispatcher::modbus::ModbusPollingPlanBuilder::validate_configuration(
            configuration
        )
    );
}

TEST(ModbusPollingConfigTests, AllowsEmptyMappingList)
{
    const auto configuration =
        make_configuration();

    EXPECT_NO_THROW(
        dispatcher::modbus::ModbusPollingPlanBuilder::validate_configuration(
            configuration
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    EXPECT_TRUE(
        plan.empty()
    );
}

TEST(ModbusPollingConfigTests, RejectsEmptyHost)
{
    auto configuration =
        make_configuration();

    configuration.endpoint.host = "";

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsZeroPort)
{
    auto configuration =
        make_configuration();

    configuration.endpoint.port = 0;

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsZeroPollInterval)
{
    auto configuration =
        make_configuration();

    configuration.poll_interval_ms = 0;

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsZeroTimeout)
{
    auto configuration =
        make_configuration();

    configuration.timeout_ms = 0;

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsZeroMaxRegistersPerRequest)
{
    auto configuration =
        make_configuration();

    configuration.max_registers_per_request = 0;

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsTooLargeMaxRegistersPerRequest)
{
    auto configuration =
        make_configuration();

    configuration.max_registers_per_request = 126;

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsEmptyTagId)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "",
            10
        )
    );

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsZeroUnitId)
{
    auto configuration =
        make_configuration();

    auto mapping =
        make_mapping(
            "pump.pressure",
            10
        );

    mapping.unit_id = 0;

    configuration.mappings.push_back(
        mapping
    );

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsUnsupportedFunctionCode)
{
    auto configuration =
        make_configuration();

    auto mapping =
        make_mapping(
            "pump.pressure",
            10
        );

    mapping.function_code =
        dispatcher::modbus::ModbusFunctionCode::read_coils;

    configuration.mappings.push_back(
        mapping
    );

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsAddressOverflow)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.energy",
            65535,
            dispatcher::modbus::ModbusRegisterValueType::uint32
        )
    );

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, RejectsDuplicateEnabledTagIds)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            11
        )
    );

    expect_validate_configuration_throws(
        configuration
    );
}

TEST(ModbusPollingConfigTests, AllowsDuplicateDisabledTagIds)
{
    auto configuration =
        make_configuration();

    auto enabled =
        make_mapping(
            "pump.pressure",
            10
        );

    auto disabled =
        make_mapping(
            "pump.pressure",
            11
        );

    disabled.enabled = false;

    configuration.mappings.push_back(
        enabled
    );

    configuration.mappings.push_back(
        disabled
    );

    EXPECT_NO_THROW(
        dispatcher::modbus::ModbusPollingPlanBuilder::validate_configuration(
            configuration
        )
    );
}

TEST(ModbusPollingConfigTests, BuildsSingleReadPlanRequest)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].unit_id,
        1
    );

    EXPECT_EQ(
        plan[0].function_code,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers
    );

    EXPECT_EQ(
        plan[0].start_address,
        10
    );

    EXPECT_EQ(
        plan[0].quantity,
        1
    );

    ASSERT_EQ(
        plan[0].mappings.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].mappings[0].tag_id,
        "pump.pressure"
    );
}

TEST(ModbusPollingConfigTests, MergesAdjacentMappingsIntoOneRequest)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "pump.temperature",
            11
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].start_address,
        10
    );

    EXPECT_EQ(
        plan[0].quantity,
        2
    );

    ASSERT_EQ(
        plan[0].mappings.size(),
        2U
    );
}

TEST(ModbusPollingConfigTests, MergesOverlappingMappingsIntoOneRequest)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.energy",
            10,
            dispatcher::modbus::ModbusRegisterValueType::uint32
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "pump.energy.high",
            11
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].start_address,
        10
    );

    EXPECT_EQ(
        plan[0].quantity,
        2
    );

    ASSERT_EQ(
        plan[0].mappings.size(),
        2U
    );
}

TEST(ModbusPollingConfigTests, DoesNotMergeDifferentUnitIds)
{
    auto configuration =
        make_configuration();

    auto first =
        make_mapping(
            "pump.pressure",
            10
        );

    auto second =
        make_mapping(
            "tank.level",
            11
        );

    second.unit_id = 2;

    configuration.mappings.push_back(
        first
    );

    configuration.mappings.push_back(
        second
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        2U
    );

    EXPECT_EQ(
        plan[0].unit_id,
        1
    );

    EXPECT_EQ(
        plan[1].unit_id,
        2
    );
}

TEST(ModbusPollingConfigTests, DoesNotMergeDifferentFunctionCodes)
{
    auto configuration =
        make_configuration();

    auto first =
        make_mapping(
            "pump.pressure",
            10
        );

    auto second =
        make_mapping(
            "tank.level",
            11
        );

    second.function_code =
        dispatcher::modbus::ModbusFunctionCode::read_input_registers;

    configuration.mappings.push_back(
        first
    );

    configuration.mappings.push_back(
        second
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        2U
    );

    EXPECT_EQ(
        plan[0].function_code,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers
    );

    EXPECT_EQ(
        plan[1].function_code,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers
    );
}

TEST(ModbusPollingConfigTests, DoesNotMergeBeyondMaxRegistersPerRequest)
{
    auto configuration =
        make_configuration();

    configuration.max_registers_per_request = 2;

    configuration.mappings.push_back(
        make_mapping(
            "tag.1",
            10
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "tag.2",
            11
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "tag.3",
            12
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        2U
    );

    EXPECT_EQ(
        plan[0].start_address,
        10
    );

    EXPECT_EQ(
        plan[0].quantity,
        2
    );

    EXPECT_EQ(
        plan[1].start_address,
        12
    );

    EXPECT_EQ(
        plan[1].quantity,
        1
    );
}

TEST(ModbusPollingConfigTests, SkipsDisabledMappingsWhenBuildingPlan)
{
    auto configuration =
        make_configuration();

    auto enabled =
        make_mapping(
            "pump.pressure",
            10
        );

    auto disabled =
        make_mapping(
            "pump.temperature",
            11
        );

    disabled.enabled = false;

    configuration.mappings.push_back(
        enabled
    );

    configuration.mappings.push_back(
        disabled
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        1U
    );

    ASSERT_EQ(
        plan[0].mappings.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].mappings[0].tag_id,
        "pump.pressure"
    );
}

TEST(ModbusPollingConfigTests, SortsMappingsBeforeBuildingPlan)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "tag.3",
            12
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "tag.1",
            10
        )
    );

    configuration.mappings.push_back(
        make_mapping(
            "tag.2",
            11
        )
    );

    const auto plan =
        dispatcher::modbus::ModbusPollingPlanBuilder::build_read_plan(
            configuration
        );

    ASSERT_EQ(
        plan.size(),
        1U
    );

    EXPECT_EQ(
        plan[0].start_address,
        10
    );

    EXPECT_EQ(
        plan[0].quantity,
        3
    );

    ASSERT_EQ(
        plan[0].mappings.size(),
        3U
    );

    EXPECT_EQ(
        plan[0].mappings[0].tag_id,
        "tag.1"
    );

    EXPECT_EQ(
        plan[0].mappings[1].tag_id,
        "tag.2"
    );

    EXPECT_EQ(
        plan[0].mappings[2].tag_id,
        "tag.3"
    );
}

TEST(ModbusPollingConfigTests, BuildPlanValidatesConfiguration)
{
    auto configuration =
        make_configuration();

    configuration.endpoint.host = "";

    expect_build_plan_throws(
        configuration
    );
}