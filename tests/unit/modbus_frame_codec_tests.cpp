#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_frame_codec.hpp>
#include <dispatcher/modbus/modbus_types.hpp>

#include <gtest/gtest.h>

namespace
{
    void expect_decode_response_throws(
        const dispatcher::modbus::ModbusBytes& frame
    )
    {
        EXPECT_THROW(
            {
                const auto response =
                    dispatcher::modbus::ModbusFrameCodec::decode_read_registers_response(
                        frame
                    );

                static_cast<void>(
                    response
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

    void expect_decode_exception_throws(
        const dispatcher::modbus::ModbusBytes& frame
    )
    {
        EXPECT_THROW(
            {
                const auto response =
                    dispatcher::modbus::ModbusFrameCodec::decode_exception_response(
                        frame
                    );

                static_cast<void>(
                    response
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

    void expect_encode_request_throws(
        const dispatcher::modbus::ModbusReadRegistersRequest& request
    )
    {
        EXPECT_THROW(
            {
                const auto frame =
                    dispatcher::modbus::ModbusFrameCodec::encode_read_registers_request(
                        request
                    );

                static_cast<void>(
                    frame
                );
            },
            dispatcher::modbus::ModbusError
        );
    }
}

TEST(ModbusFrameCodecTests, EncodesReadHoldingRegistersRequest)
{
    const dispatcher::modbus::ModbusReadRegistersRequest request{
        0x1234,
        0x11,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers,
        0x006B,
        0x0003
    };

    const auto frame =
        dispatcher::modbus::ModbusFrameCodec::encode_read_registers_request(
            request
        );

    const dispatcher::modbus::ModbusBytes expected{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x06,
        0x11,
        0x03,
        0x00, 0x6B,
        0x00, 0x03
    };

    EXPECT_EQ(
        frame,
        expected
    );
}

TEST(ModbusFrameCodecTests, EncodesReadInputRegistersRequest)
{
    const dispatcher::modbus::ModbusReadRegistersRequest request{
        0x0001,
        0x01,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers,
        0x0000,
        0x0002
    };

    const auto frame =
        dispatcher::modbus::ModbusFrameCodec::encode_read_registers_request(
            request
        );

    const dispatcher::modbus::ModbusBytes expected{
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x06,
        0x01,
        0x04,
        0x00, 0x00,
        0x00, 0x02
    };

    EXPECT_EQ(
        frame,
        expected
    );
}

TEST(ModbusFrameCodecTests, RejectsUnsupportedReadRequestFunction)
{
    dispatcher::modbus::ModbusReadRegistersRequest request{
        1,
        1,
        dispatcher::modbus::ModbusFunctionCode::read_coils,
        0,
        1
    };

    expect_encode_request_throws(
        request
    );
}

TEST(ModbusFrameCodecTests, RejectsZeroUnitId)
{
    dispatcher::modbus::ModbusReadRegistersRequest request{
        1,
        0,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers,
        0,
        1
    };

    expect_encode_request_throws(
        request
    );
}

TEST(ModbusFrameCodecTests, RejectsZeroQuantity)
{
    dispatcher::modbus::ModbusReadRegistersRequest request{
        1,
        1,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers,
        0,
        0
    };

    expect_encode_request_throws(
        request
    );
}

TEST(ModbusFrameCodecTests, RejectsTooLargeQuantity)
{
    dispatcher::modbus::ModbusReadRegistersRequest request{
        1,
        1,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers,
        0,
        126
    };

    expect_encode_request_throws(
        request
    );
}

TEST(ModbusFrameCodecTests, DecodesReadHoldingRegistersResponse)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x09,
        0x11,
        0x03,
        0x06,
        0x02, 0x2B,
        0x00, 0x00,
        0x00, 0x64
    };

    const auto response =
        dispatcher::modbus::ModbusFrameCodec::decode_read_registers_response(
            frame
        );

    EXPECT_EQ(
        response.transaction_id,
        0x1234
    );

    EXPECT_EQ(
        response.unit_id,
        0x11
    );

    EXPECT_EQ(
        response.function_code,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers
    );

    const std::vector<std::uint16_t> expected_registers{
        0x022B,
        0x0000,
        0x0064
    };

    EXPECT_EQ(
        response.registers,
        expected_registers
    );
}

TEST(ModbusFrameCodecTests, DecodesReadInputRegistersResponse)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x00, 0x02,
        0x00, 0x00,
        0x00, 0x07,
        0x01,
        0x04,
        0x04,
        0x00, 0x0A,
        0x00, 0x14
    };

    const auto response =
        dispatcher::modbus::ModbusFrameCodec::decode_read_registers_response(
            frame
        );

    EXPECT_EQ(
        response.transaction_id,
        2
    );

    EXPECT_EQ(
        response.unit_id,
        1
    );

    EXPECT_EQ(
        response.function_code,
        dispatcher::modbus::ModbusFunctionCode::read_input_registers
    );

    const std::vector<std::uint16_t> expected_registers{
        10,
        20
    };

    EXPECT_EQ(
        response.registers,
        expected_registers
    );
}

TEST(ModbusFrameCodecTests, DetectsExceptionResponse)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x03,
        0x11,
        0x83,
        0x02
    };

    EXPECT_TRUE(
        dispatcher::modbus::ModbusFrameCodec::is_exception_response(
            frame
        )
    );
}

TEST(ModbusFrameCodecTests, DecodesExceptionResponse)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x03,
        0x11,
        0x83,
        0x02
    };

    const auto response =
        dispatcher::modbus::ModbusFrameCodec::decode_exception_response(
            frame
        );

    EXPECT_EQ(
        response.transaction_id,
        0x1234
    );

    EXPECT_EQ(
        response.unit_id,
        0x11
    );

    EXPECT_EQ(
        response.function_code,
        dispatcher::modbus::ModbusFunctionCode::read_holding_registers
    );

    EXPECT_EQ(
        response.exception_code,
        0x02
    );
}

TEST(ModbusFrameCodecTests, DecodeReadRegistersRejectsExceptionResponse)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x03,
        0x11,
        0x83,
        0x02
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsShortResponseFrame)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x00,
        0x01
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsInvalidProtocolId)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x01,
        0x00, 0x07,
        0x11,
        0x03,
        0x04,
        0x00, 0x01,
        0x00, 0x02
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsMismatchedMbapLength)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x08,
        0x11,
        0x03,
        0x04,
        0x00, 0x01,
        0x00, 0x02
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsOddByteCount)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x06,
        0x11,
        0x03,
        0x03,
        0x00, 0x01,
        0x00
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsMismatchedByteCount)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x07,
        0x11,
        0x03,
        0x06,
        0x00, 0x01,
        0x00, 0x02
    };

    expect_decode_response_throws(
        frame
    );
}

TEST(ModbusFrameCodecTests, RejectsNonExceptionFrameWhenDecodingException)
{
    const dispatcher::modbus::ModbusBytes frame{
        0x12, 0x34,
        0x00, 0x00,
        0x00, 0x07,
        0x11,
        0x03,
        0x04,
        0x00, 0x01,
        0x00, 0x02
    };

    expect_decode_exception_throws(
        frame
    );
}