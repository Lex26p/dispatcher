#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_frame_codec.hpp>
#include <dispatcher/modbus/modbus_simulated_tcp_transport.hpp>
#include <dispatcher/modbus/modbus_tcp_client.hpp>
#include <dispatcher/modbus/modbus_telemetry_adapter.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{
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

    void expect_exchange_throws(
        dispatcher::modbus::ModbusSimulatedTcpTransport& transport,
        const dispatcher::modbus::ModbusBytes& request
    )
    {
        EXPECT_THROW(
            {
                const auto response =
                    transport.exchange(
                        request
                    );

                static_cast<void>(
                    response
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

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
}

TEST(ModbusSimulatedTcpTransportTests, RespondsToHoldingRegisterRead)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_registers(
        1,
        10,
        {
            100,
            200
        }
    );

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_holding_registers(
            1,
            10,
            2
        );

    ASSERT_EQ(
        response.registers.size(),
        2U
    );

    EXPECT_EQ(
        response.registers[0],
        100
    );

    EXPECT_EQ(
        response.registers[1],
        200
    );

    EXPECT_EQ(
        transport.exchange_count(),
        1
    );

    ASSERT_EQ(
        transport.requests().size(),
        1U
    );
}

TEST(ModbusSimulatedTcpTransportTests, RespondsToInputRegisterRead)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_input_registers(
        1,
        0,
        {
            11,
            22,
            33
        }
    );

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_input_registers(
            1,
            0,
            3
        );

    ASSERT_EQ(
        response.registers.size(),
        3U
    );

    EXPECT_EQ(
        response.function_code,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers
    );

    EXPECT_EQ(
        response.registers[0],
        11
    );

    EXPECT_EQ(
        response.registers[1],
        22
    );

    EXPECT_EQ(
        response.registers[2],
        33
    );
}

TEST(ModbusSimulatedTcpTransportTests, MissingRegisterProducesModbusException)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_register(
        1,
        10,
        100
    );

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    EXPECT_THROW(
        {
            const auto response =
                client.read_holding_registers(
                    1,
                    10,
                    2
                );

            static_cast<void>(
                response
            );
        },
        dispatcher::modbus::ModbusError
    );

    EXPECT_EQ(
        transport.exchange_count(),
        1
    );
}

TEST(ModbusSimulatedTcpTransportTests, UnsupportedFunctionReturnsExceptionFrame)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    const dispatcher::modbus::ModbusBytes request{
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x06,
        0x01,
        0x01,
        0x00, 0x00,
        0x00, 0x01
    };

    const auto response =
        transport.exchange(
            request
        );

    EXPECT_TRUE(
        dispatcher::modbus::ModbusFrameCodec::is_exception_response(
            response
        )
    );

    const auto exception =
        dispatcher::modbus::ModbusFrameCodec::decode_exception_response(
            response
        );

    EXPECT_EQ(
        exception.transaction_id,
        1
    );

    EXPECT_EQ(
        exception.unit_id,
        1
    );

    EXPECT_EQ(
        exception.exception_code,
        0x01
    );
}

TEST(ModbusSimulatedTcpTransportTests, MalformedRequestThrows)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    const dispatcher::modbus::ModbusBytes request{
        0x00,
        0x01
    };

    expect_exchange_throws(
        transport,
        request
    );
}

TEST(ModbusSimulatedTcpTransportTests, InvalidProtocolIdThrows)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    const dispatcher::modbus::ModbusBytes request{
        0x00, 0x01,
        0x00, 0x01,
        0x00, 0x06,
        0x01,
        0x03,
        0x00, 0x00,
        0x00, 0x01
    };

    expect_exchange_throws(
        transport,
        request
    );
}

TEST(ModbusSimulatedTcpTransportTests, ClearRemovesRegistersAndRequests)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_register(
        1,
        10,
        100
    );

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_holding_registers(
            1,
            10,
            1
        );

    static_cast<void>(
        response
        );

    EXPECT_EQ(
        transport.exchange_count(),
        1
    );

    ASSERT_EQ(
        transport.requests().size(),
        1U
    );

    transport.clear();

    EXPECT_EQ(
        transport.exchange_count(),
        0
    );

    EXPECT_TRUE(
        transport.requests().empty()
    );

    EXPECT_THROW(
        {
            const auto after_clear =
                client.read_holding_registers(
                    1,
                    10,
                    1
                );

            static_cast<void>(
                after_clear
            );
        },
        dispatcher::modbus::ModbusError
    );
}

TEST(ModbusSimulatedTcpTransportTests, AdapterSmokePollsHoldingRegisters)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_registers(
        1,
        10,
        {
            100,
            250
        }
    );

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

TEST(ModbusSimulatedTcpTransportTests, AdapterSmokePollsFloat32LowWordFirst)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_registers(
        1,
        20,
        {
            0x0000,
            0x4148
        }
    );

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
}

TEST(ModbusSimulatedTcpTransportTests, AdapterRecordsExceptionFromSimulator)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_register(
        1,
        10,
        100
    );

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
        2
    );
}

TEST(ModbusSimulatedTcpTransportTests, RequestFrameContainsExpectedStartAndQuantity)
{
    dispatcher::modbus::ModbusSimulatedTcpTransport transport;

    transport.set_holding_registers(
        1,
        100,
        {
            1,
            2,
            3
        }
    );

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_holding_registers(
            1,
            100,
            3
        );

    static_cast<void>(
        response
        );

    ASSERT_EQ(
        transport.requests().size(),
        1U
    );

    const auto& request =
        transport.requests()[0];

    EXPECT_EQ(
        request[7],
        0x03
    );

    EXPECT_EQ(
        read_u16_be(
            request,
            8
        ),
        100
    );

    EXPECT_EQ(
        read_u16_be(
            request,
            10
        ),
        3
    );
}