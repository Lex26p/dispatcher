#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_telemetry_adapter.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace
{
    class ScriptedModbusTcpTransport final
        : public dispatcher::modbus::IModbusTcpTransport
    {
    public:
        using Handler =
            std::function<dispatcher::modbus::ModbusBytes(
                const dispatcher::modbus::ModbusBytes&
            )>;

        explicit ScriptedModbusTcpTransport(
            Handler handler
        )
            : handler_(
                std::move(
                    handler
                )
            )
        {
        }

        [[nodiscard]] dispatcher::modbus::ModbusBytes exchange(
            const dispatcher::modbus::ModbusBytes& request_frame
        ) override
        {
            ++exchange_count_;

            requests.push_back(
                request_frame
            );

            return handler_(
                request_frame
            );
        }

        [[nodiscard]] int exchange_count() const noexcept
        {
            return exchange_count_;
        }

        std::vector<dispatcher::modbus::ModbusBytes> requests{};

    private:
        Handler handler_;
        int exchange_count_{ 0 };
    };

    [[nodiscard]] std::uint16_t read_u16_be(
        const dispatcher::modbus::ModbusBytes& bytes,
        std::size_t offset
    )
    {
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

    [[nodiscard]] dispatcher::modbus::ModbusBytes make_read_response(
        std::uint16_t transaction_id,
        std::uint8_t unit_id,
        std::uint8_t function_code,
        const std::vector<std::uint16_t>& registers
    )
    {
        dispatcher::modbus::ModbusBytes frame;

        const auto byte_count =
            static_cast<std::uint8_t>(
                registers.size() * 2U
                );

        const auto length =
            static_cast<std::uint16_t>(
                3U + byte_count
                );

        frame.push_back(
            static_cast<std::uint8_t>(
                (transaction_id >> 8U) & 0xFFU
                )
        );

        frame.push_back(
            static_cast<std::uint8_t>(
                transaction_id & 0xFFU
                )
        );

        frame.push_back(
            0x00
        );

        frame.push_back(
            0x00
        );

        frame.push_back(
            static_cast<std::uint8_t>(
                (length >> 8U) & 0xFFU
                )
        );

        frame.push_back(
            static_cast<std::uint8_t>(
                length & 0xFFU
                )
        );

        frame.push_back(
            unit_id
        );

        frame.push_back(
            function_code
        );

        frame.push_back(
            byte_count
        );

        for (const auto value : registers)
        {
            frame.push_back(
                static_cast<std::uint8_t>(
                    (value >> 8U) & 0xFFU
                    )
            );

            frame.push_back(
                static_cast<std::uint8_t>(
                    value & 0xFFU
                    )
            );
        }

        return frame;
    }

    [[nodiscard]] dispatcher::modbus::ModbusBytes make_exception_response(
        std::uint16_t transaction_id,
        std::uint8_t unit_id,
        std::uint8_t function_code,
        std::uint8_t exception_code
    )
    {
        return dispatcher::modbus::ModbusBytes{
            static_cast<std::uint8_t>(
                (transaction_id >> 8U) & 0xFFU
            ),
            static_cast<std::uint8_t>(
                transaction_id & 0xFFU
            ),
            0x00,
            0x00,
            0x00,
            0x03,
            unit_id,
            static_cast<std::uint8_t>(
                function_code | 0x80U
            ),
            exception_code
        };
    }

    [[nodiscard]] dispatcher::modbus::ModbusPollingConfiguration make_configuration()
    {
        dispatcher::modbus::ModbusPollingConfiguration configuration;

        configuration.endpoint.host = "127.0.0.1";
        configuration.endpoint.port = 502;
        configuration.poll_interval_ms = 1000;
        configuration.timeout_ms = 500;
        configuration.max_registers_per_request = 125;

        return configuration;
    }

    [[nodiscard]] dispatcher::modbus::ModbusTagMapping make_mapping(
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
}

TEST(ModbusTelemetryAdapterTests, BuildsReadPlanOnConstruction)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return dispatcher::modbus::ModbusBytes{};
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    ASSERT_EQ(
        adapter.read_plan().size(),
        1U
    );

    EXPECT_EQ(
        adapter.read_plan()[0].start_address,
        10
    );

    EXPECT_EQ(
        adapter.read_plan()[0].quantity,
        1
    );
}

TEST(ModbusTelemetryAdapterTests, PollsSingleUInt16Mapping)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x03,
                {
                    123
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_EQ(
        result.samples[0].tag_id,
        "pump.pressure"
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        123.0
    );

    EXPECT_EQ(
        result.samples[0].quality,
        "good"
    );

    EXPECT_EQ(
        result.samples[0].source,
        "modbus-tcp"
    );

    EXPECT_EQ(
        transport.exchange_count(),
        1
    );
}

TEST(ModbusTelemetryAdapterTests, AppliesScaleAndOffset)
{
    auto configuration =
        make_configuration();

    auto mapping =
        make_mapping(
            "pump.pressure",
            10
        );

    mapping.scale = 0.1;
    mapping.offset = 5.0;

    configuration.mappings.push_back(
        mapping
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x03,
                {
                    100
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        15.0
    );
}

TEST(ModbusTelemetryAdapterTests, PollsMergedMappingsFromSingleResponse)
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

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x03,
                {
                    100,
                    250
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    ASSERT_EQ(
        adapter.read_plan().size(),
        1U
    );

    EXPECT_EQ(
        adapter.read_plan()[0].quantity,
        2
    );

    const auto result =
        adapter.poll_once();

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        result.samples.size(),
        2U
    );

    EXPECT_EQ(
        result.samples[0].tag_id,
        "pump.pressure"
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        100.0
    );

    EXPECT_EQ(
        result.samples[1].tag_id,
        "pump.temperature"
    );

    EXPECT_DOUBLE_EQ(
        result.samples[1].value,
        250.0
    );

    EXPECT_EQ(
        transport.exchange_count(),
        1
    );
}

TEST(ModbusTelemetryAdapterTests, PollsFloat32LowWordFirst)
{
    auto configuration =
        make_configuration();

    auto mapping =
        make_mapping(
            "pump.flow",
            20,
            dispatcher::modbus::ModbusRegisterValueType::float32
        );

    mapping.word_order =
        dispatcher::modbus::ModbusWordOrder::low_word_first;

    configuration.mappings.push_back(
        mapping
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x03,
                {
                    0x0000,
                    0x4148
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        12.5
    );

    EXPECT_EQ(
        result.samples[0].value_type,
        dispatcher::modbus::ModbusRegisterValueType::float32
    );
}

TEST(ModbusTelemetryAdapterTests, PollsInputRegisters)
{
    auto configuration =
        make_configuration();

    auto mapping =
        make_mapping(
            "tank.level",
            0
        );

    mapping.function_code =
        dispatcher::modbus::ModbusFunctionCode::read_input_registers;

    configuration.mappings.push_back(
        mapping
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x04,
                {
                    77
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_TRUE(
        result.success()
    );

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_EQ(
        result.samples[0].function_code,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        77.0
    );
}

TEST(ModbusTelemetryAdapterTests, SkipsDisabledMappings)
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

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_read_response(
                transaction_id,
                1,
                0x03,
                {
                    100
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    ASSERT_EQ(
        adapter.read_plan().size(),
        1U
    );

    ASSERT_EQ(
        adapter.read_plan()[0].mappings.size(),
        1U
    );

    const auto result =
        adapter.poll_once();

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_EQ(
        result.samples[0].tag_id,
        "pump.pressure"
    );
}

TEST(ModbusTelemetryAdapterTests, EmptyConfigurationProducesSuccessfulEmptyResult)
{
    const auto configuration =
        make_configuration();

    ScriptedModbusTcpTransport transport{
    [](const dispatcher::modbus::ModbusBytes&) -> dispatcher::modbus::ModbusBytes
    {
        throw std::runtime_error{
            "transport should not be called"
        };
    }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_TRUE(
        result.success()
    );

    EXPECT_FALSE(
        result.has_samples()
    );

    EXPECT_TRUE(
        result.samples.empty()
    );

    EXPECT_TRUE(
        result.errors.empty()
    );

    EXPECT_EQ(
        transport.exchange_count(),
        0
    );
}

TEST(ModbusTelemetryAdapterTests, RecordsTransportError)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&) -> dispatcher::modbus::ModbusBytes
        {
            throw std::runtime_error{
                "network unavailable"
            };
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        result.samples.empty()
    );

    ASSERT_EQ(
        result.errors.size(),
        1U
    );

    EXPECT_EQ(
        result.errors[0].start_address,
        10
    );

    EXPECT_EQ(
        result.errors[0].quantity,
        1
    );

    EXPECT_EQ(
        result.errors[0].message,
        "network unavailable"
    );
}

TEST(ModbusTelemetryAdapterTests, RecordsModbusExceptionAsError)
{
    auto configuration =
        make_configuration();

    configuration.mappings.push_back(
        make_mapping(
            "pump.pressure",
            10
        )
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            return make_exception_response(
                transaction_id,
                1,
                0x03,
                0x02
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_FALSE(
        result.success()
    );

    EXPECT_TRUE(
        result.samples.empty()
    );

    ASSERT_EQ(
        result.errors.size(),
        1U
    );

    EXPECT_EQ(
        result.errors[0].unit_id,
        1
    );

    EXPECT_EQ(
        result.errors[0].function_code,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers
    );
}

TEST(ModbusTelemetryAdapterTests, ContinuesAfterOneRequestFails)
{
    auto configuration =
        make_configuration();

    configuration.max_registers_per_request = 1;

    auto first =
        make_mapping(
            "pump.pressure",
            10
        );

    first.unit_id = 1;

    auto second =
        make_mapping(
            "tank.level",
            20
        );

    second.unit_id = 2;

    configuration.mappings.push_back(
        first
    );

    configuration.mappings.push_back(
        second
    );

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes& request)
        {
            const auto unit_id =
                request[6];

            const auto transaction_id =
                read_u16_be(
                    request,
                    0
                );

            if (unit_id == 1)
            {
                throw std::runtime_error{
                    "unit 1 offline"
                };
            }

            return make_read_response(
                transaction_id,
                unit_id,
                0x03,
                {
                    55
                }
            );
        }
    };

    dispatcher::modbus::ModbusTelemetryAdapter adapter{
        configuration,
        transport
    };

    const auto result =
        adapter.poll_once();

    EXPECT_FALSE(
        result.success()
    );

    ASSERT_EQ(
        result.errors.size(),
        1U
    );

    ASSERT_EQ(
        result.samples.size(),
        1U
    );

    EXPECT_EQ(
        result.samples[0].tag_id,
        "tank.level"
    );

    EXPECT_DOUBLE_EQ(
        result.samples[0].value,
        55.0
    );

    EXPECT_EQ(
        transport.exchange_count(),
        2
    );
}

TEST(ModbusTelemetryAdapterTests, InvalidConfigurationThrowsOnConstruction)
{
    auto configuration =
        make_configuration();

    configuration.endpoint.host = "";

    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return dispatcher::modbus::ModbusBytes{};
        }
    };

    auto create_adapter =
        [&configuration, &transport]()
        {
            dispatcher::modbus::ModbusTelemetryAdapter adapter(
                configuration,
                transport
            );

            static_cast<void>(
                adapter
                );
        };

    EXPECT_THROW(
        create_adapter(),
        dispatcher::modbus::ModbusError
    );
}