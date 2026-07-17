#include <dispatcher/modbus/modbus_error.hpp>
#include <dispatcher/modbus/modbus_register_decoder.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{
    void expect_decode_uint16_throws(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset
    )
    {
        EXPECT_THROW(
            {
                const auto value =
                    dispatcher::modbus::ModbusRegisterDecoder::decode_uint16(
                        registers,
                        offset
                    );

                static_cast<void>(
                    value
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

    void expect_decode_uint32_throws(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset
    )
    {
        EXPECT_THROW(
            {
                const auto value =
                    dispatcher::modbus::ModbusRegisterDecoder::decode_uint32(
                        registers,
                        offset
                    );

                static_cast<void>(
                    value
                );
            },
            dispatcher::modbus::ModbusError
        );
    }

    void expect_decode_numeric_throws(
        const std::vector<std::uint16_t>& registers,
        dispatcher::modbus::ModbusRegisterValueType value_type,
        std::size_t offset
    )
    {
        EXPECT_THROW(
            {
                const auto value =
                    dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
                        registers,
                        value_type,
                        offset
                    );

                static_cast<void>(
                    value
                );
            },
            dispatcher::modbus::ModbusError
        );
    }
}

TEST(ModbusRegisterDecoderTests, DecodesUInt16)
{
    const std::vector<std::uint16_t> registers{
        0x0001,
        0x1234
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_uint16(
            registers,
            1
        );

    EXPECT_EQ(
        value,
        0x1234
    );
}

TEST(ModbusRegisterDecoderTests, DecodesInt16Positive)
{
    const std::vector<std::uint16_t> registers{
        0x007B
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_int16(
            registers
        );

    EXPECT_EQ(
        value,
        123
    );
}

TEST(ModbusRegisterDecoderTests, DecodesInt16Negative)
{
    const std::vector<std::uint16_t> registers{
        0xFF85
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_int16(
            registers
        );

    EXPECT_EQ(
        value,
        -123
    );
}

TEST(ModbusRegisterDecoderTests, DecodesUInt32HighWordFirst)
{
    const std::vector<std::uint16_t> registers{
        0x1234,
        0x5678
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_uint32(
            registers,
            0,
            dispatcher::modbus::ModbusWordOrder::high_word_first
        );

    EXPECT_EQ(
        value,
        0x12345678U
    );
}

TEST(ModbusRegisterDecoderTests, DecodesUInt32LowWordFirst)
{
    const std::vector<std::uint16_t> registers{
        0x5678,
        0x1234
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_uint32(
            registers,
            0,
            dispatcher::modbus::ModbusWordOrder::low_word_first
        );

    EXPECT_EQ(
        value,
        0x12345678U
    );
}

TEST(ModbusRegisterDecoderTests, DecodesInt32Positive)
{
    const std::vector<std::uint16_t> registers{
        0x0000,
        0x007B
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_int32(
            registers
        );

    EXPECT_EQ(
        value,
        123
    );
}

TEST(ModbusRegisterDecoderTests, DecodesInt32Negative)
{
    const std::vector<std::uint16_t> registers{
        0xFFFF,
        0xFF85
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_int32(
            registers
        );

    EXPECT_EQ(
        value,
        -123
    );
}

TEST(ModbusRegisterDecoderTests, DecodesFloat32HighWordFirst)
{
    const std::vector<std::uint16_t> registers{
        0x4148,
        0x0000
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_float32(
            registers,
            0,
            dispatcher::modbus::ModbusWordOrder::high_word_first
        );

    EXPECT_FLOAT_EQ(
        value,
        12.5F
    );
}

TEST(ModbusRegisterDecoderTests, DecodesFloat32LowWordFirst)
{
    const std::vector<std::uint16_t> registers{
        0x0000,
        0x4148
    };

    const auto value =
        dispatcher::modbus::ModbusRegisterDecoder::decode_float32(
            registers,
            0,
            dispatcher::modbus::ModbusWordOrder::low_word_first
        );

    EXPECT_FLOAT_EQ(
        value,
        12.5F
    );
}

TEST(ModbusRegisterDecoderTests, DecodeNumericUInt16)
{
    const std::vector<std::uint16_t> registers{
        0x002A
    };

    const auto decoded =
        dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
            registers,
            dispatcher::modbus::ModbusRegisterValueType::uint16
        );

    EXPECT_EQ(
        decoded.value_type,
        dispatcher::modbus::ModbusRegisterValueType::uint16
    );

    EXPECT_DOUBLE_EQ(
        decoded.numeric_value,
        42.0
    );
}

TEST(ModbusRegisterDecoderTests, DecodeNumericInt16)
{
    const std::vector<std::uint16_t> registers{
        0xFFD6
    };

    const auto decoded =
        dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
            registers,
            dispatcher::modbus::ModbusRegisterValueType::int16
        );

    EXPECT_EQ(
        decoded.value_type,
        dispatcher::modbus::ModbusRegisterValueType::int16
    );

    EXPECT_DOUBLE_EQ(
        decoded.numeric_value,
        -42.0
    );
}

TEST(ModbusRegisterDecoderTests, DecodeNumericUInt32)
{
    const std::vector<std::uint16_t> registers{
        0x0001,
        0x0002
    };

    const auto decoded =
        dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
            registers,
            dispatcher::modbus::ModbusRegisterValueType::uint32
        );

    EXPECT_EQ(
        decoded.value_type,
        dispatcher::modbus::ModbusRegisterValueType::uint32
    );

    EXPECT_DOUBLE_EQ(
        decoded.numeric_value,
        65538.0
    );
}

TEST(ModbusRegisterDecoderTests, DecodeNumericInt32)
{
    const std::vector<std::uint16_t> registers{
        0xFFFF,
        0xFFD6
    };

    const auto decoded =
        dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
            registers,
            dispatcher::modbus::ModbusRegisterValueType::int32
        );

    EXPECT_EQ(
        decoded.value_type,
        dispatcher::modbus::ModbusRegisterValueType::int32
    );

    EXPECT_DOUBLE_EQ(
        decoded.numeric_value,
        -42.0
    );
}

TEST(ModbusRegisterDecoderTests, DecodeNumericFloat32)
{
    const std::vector<std::uint16_t> registers{
        0x4148,
        0x0000
    };

    const auto decoded =
        dispatcher::modbus::ModbusRegisterDecoder::decode_numeric(
            registers,
            dispatcher::modbus::ModbusRegisterValueType::float32
        );

    EXPECT_EQ(
        decoded.value_type,
        dispatcher::modbus::ModbusRegisterValueType::float32
    );

    EXPECT_DOUBLE_EQ(
        decoded.numeric_value,
        12.5
    );
}

TEST(ModbusRegisterDecoderTests, RejectsEmptyRegistersForUInt16)
{
    const std::vector<std::uint16_t> registers{};

    expect_decode_uint16_throws(
        registers,
        0
    );
}

TEST(ModbusRegisterDecoderTests, RejectsOffsetOutsideBuffer)
{
    const std::vector<std::uint16_t> registers{
        0x0001
    };

    expect_decode_uint16_throws(
        registers,
        1
    );
}

TEST(ModbusRegisterDecoderTests, RejectsTooSmallBufferForUInt32)
{
    const std::vector<std::uint16_t> registers{
        0x0001
    };

    expect_decode_uint32_throws(
        registers,
        0
    );
}

TEST(ModbusRegisterDecoderTests, RejectsTooSmallBufferForFloat32AtOffset)
{
    const std::vector<std::uint16_t> registers{
        0x0000,
        0x4148
    };

    expect_decode_numeric_throws(
        registers,
        dispatcher::modbus::ModbusRegisterValueType::float32,
        1
    );
}