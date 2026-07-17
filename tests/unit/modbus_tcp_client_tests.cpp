#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_tcp_client.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
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

    void expect_read_holding_throws(
        dispatcher::modbus::ModbusTcpClient& client,
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity
    )
    {
        EXPECT_THROW(
            {
                const auto response =
                    client.read_holding_registers(
                        unit_id,
                        start_address,
                        quantity
                    );

                static_cast<void>(
                    response
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

    void expect_read_input_throws(
        dispatcher::modbus::ModbusTcpClient& client,
        std::uint8_t unit_id,
        std::uint16_t start_address,
        std::uint16_t quantity
    )
    {
        EXPECT_THROW(
            {
                const auto response =
                    client.read_input_registers(
                        unit_id,
                        start_address,
                        quantity
                    );

                static_cast<void>(
                    response
                );
            },
            dispatcher::modbus::ModbusError
        );
    }
}

TEST(ModbusTcpClientTests, ReadsHoldingRegistersThroughTransport)
{
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
                0x11,
                0x03,
                {
                    10,
                    20
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_holding_registers(
            0x11,
            0x006B,
            2
        );

    ASSERT_EQ(
        response.registers.size(),
        2U
    );

    EXPECT_EQ(
        response.registers[0],
        10
    );

    EXPECT_EQ(
        response.registers[1],
        20
    );

    const dispatcher::modbus::ModbusBytes expected_request{
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x06,
        0x11,
        0x03,
        0x00, 0x6B,
        0x00, 0x02
    };

    ASSERT_EQ(
        transport.requests.size(),
        1U
    );

    EXPECT_EQ(
        transport.requests[0],
        expected_request
    );
}

TEST(ModbusTcpClientTests, ReadsInputRegistersThroughTransport)
{
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
                0x01,
                0x04,
                {
                    100,
                    200
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto response =
        client.read_input_registers(
            0x01,
            0x0000,
            2
        );

    ASSERT_EQ(
        response.registers.size(),
        2U
    );

    EXPECT_EQ(
        response.function_code,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers
    );

    EXPECT_EQ(
        response.registers[0],
        100
    );

    EXPECT_EQ(
        response.registers[1],
        200
    );

    const dispatcher::modbus::ModbusBytes expected_request{
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x06,
        0x01,
        0x04,
        0x00, 0x00,
        0x00, 0x02
    };

    ASSERT_EQ(
        transport.requests.size(),
        1U
    );

    EXPECT_EQ(
        transport.requests[0],
        expected_request
    );
}

TEST(ModbusTcpClientTests, IncrementsTransactionId)
{
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
                0x01,
                0x03,
                {
                    1
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    const auto first =
        client.read_holding_registers(
            0x01,
            0,
            1
        );

    static_cast<void>(
        first
        );

    const auto second =
        client.read_holding_registers(
            0x01,
            1,
            1
        );

    static_cast<void>(
        second
        );

    ASSERT_EQ(
        transport.requests.size(),
        2U
    );

    EXPECT_EQ(
        read_u16_be(
            transport.requests[0],
            0
        ),
        1
    );

    EXPECT_EQ(
        read_u16_be(
            transport.requests[1],
            0
        ),
        2
    );

    EXPECT_EQ(
        client.next_transaction_id(),
        3
    );
}

TEST(ModbusTcpClientTests, RejectsMismatchedTransactionId)
{
    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return make_read_response(
                2,
                0x01,
                0x03,
                {
                    1
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsMismatchedUnitId)
{
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
                0x02,
                0x03,
                {
                    1
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsMismatchedFunctionCode)
{
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
                0x01,
                0x04,
                {
                    1
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsUnexpectedRegisterCount)
{
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
                0x01,
                0x03,
                {
                    1,
                    2
                }
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsExceptionResponse)
{
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
                0x01,
                0x03,
                0x02
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsExceptionResponseWithMismatchedTransactionId)
{
    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return make_exception_response(
                2,
                0x01,
                0x03,
                0x02
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsExceptionResponseWithMismatchedUnitId)
{
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
                0x02,
                0x03,
                0x02
            );
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, InvalidRequestDoesNotCallTransport)
{
    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return dispatcher::modbus::ModbusBytes{};
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0,
        0,
        1
    );

    EXPECT_EQ(
        transport.exchange_count(),
        0
    );
}

TEST(ModbusTcpClientTests, RejectsInvalidResponseFrameFromTransport)
{
    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return dispatcher::modbus::ModbusBytes{
                0x00,
                0x01
            };
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_holding_throws(
        client,
        0x01,
        0,
        1
    );
}

TEST(ModbusTcpClientTests, RejectsInvalidInputRegisterResponseFrameFromTransport)
{
    ScriptedModbusTcpTransport transport{
        [](const dispatcher::modbus::ModbusBytes&)
        {
            return dispatcher::modbus::ModbusBytes{
                0x00,
                0x01
            };
        }
    };

    dispatcher::modbus::ModbusTcpClient client{
        transport
    };

    expect_read_input_throws(
        client,
        0x01,
        0,
        1
    );
}