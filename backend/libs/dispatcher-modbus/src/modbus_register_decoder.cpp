#include <dispatcher/modbus/modbus_register_decoder.hpp>

#include <dispatcher/modbus/modbus_error.hpp>

#include <bit>
#include <cstddef>
#include <string>

namespace dispatcher::modbus
{
    std::uint16_t ModbusRegisterDecoder::decode_uint16(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset
    )
    {
        require_registers(
            registers,
            offset,
            1
        );

        return registers[offset];
    }

    std::int16_t ModbusRegisterDecoder::decode_int16(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset
    )
    {
        require_registers(
            registers,
            offset,
            1
        );

        return static_cast<std::int16_t>(
            registers[offset]
            );
    }

    std::uint32_t ModbusRegisterDecoder::decode_uint32(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset,
        ModbusWordOrder word_order
    )
    {
        require_registers(
            registers,
            offset,
            2
        );

        return compose_u32(
            registers[offset],
            registers[offset + 1],
            word_order
        );
    }

    std::int32_t ModbusRegisterDecoder::decode_int32(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset,
        ModbusWordOrder word_order
    )
    {
        const auto value =
            decode_uint32(
                registers,
                offset,
                word_order
            );

        return static_cast<std::int32_t>(
            value
            );
    }

    float ModbusRegisterDecoder::decode_float32(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset,
        ModbusWordOrder word_order
    )
    {
        const auto raw_value =
            decode_uint32(
                registers,
                offset,
                word_order
            );

        return std::bit_cast<float>(
            raw_value
        );
    }

    ModbusDecodedRegisterValue ModbusRegisterDecoder::decode_numeric(
        const std::vector<std::uint16_t>& registers,
        ModbusRegisterValueType value_type,
        std::size_t offset,
        ModbusWordOrder word_order
    )
    {
        ModbusDecodedRegisterValue decoded;

        decoded.value_type =
            value_type;

        switch (value_type)
        {
        case ModbusRegisterValueType::uint16:
            decoded.numeric_value =
                static_cast<double>(
                    decode_uint16(
                        registers,
                        offset
                    )
                    );
            break;

        case ModbusRegisterValueType::int16:
            decoded.numeric_value =
                static_cast<double>(
                    decode_int16(
                        registers,
                        offset
                    )
                    );
            break;

        case ModbusRegisterValueType::uint32:
            decoded.numeric_value =
                static_cast<double>(
                    decode_uint32(
                        registers,
                        offset,
                        word_order
                    )
                    );
            break;

        case ModbusRegisterValueType::int32:
            decoded.numeric_value =
                static_cast<double>(
                    decode_int32(
                        registers,
                        offset,
                        word_order
                    )
                    );
            break;

        case ModbusRegisterValueType::float32:
            decoded.numeric_value =
                static_cast<double>(
                    decode_float32(
                        registers,
                        offset,
                        word_order
                    )
                    );
            break;
        }

        return decoded;
    }

    void ModbusRegisterDecoder::require_registers(
        const std::vector<std::uint16_t>& registers,
        std::size_t offset,
        std::size_t required_count
    )
    {
        if (required_count == 0)
        {
            throw ModbusError(
                "Modbus decoder required register count must be positive."
            );
        }

        if (offset >= registers.size())
        {
            throw ModbusError(
                "Modbus decoder offset is outside register buffer."
            );
        }

        if (registers.size() - offset < required_count)
        {
            throw ModbusError(
                "Modbus decoder register buffer is too small."
            );
        }
    }

    std::uint32_t ModbusRegisterDecoder::compose_u32(
        std::uint16_t first_word,
        std::uint16_t second_word,
        ModbusWordOrder word_order
    )
    {
        std::uint16_t high_word =
            first_word;

        std::uint16_t low_word =
            second_word;

        if (word_order == ModbusWordOrder::low_word_first)
        {
            high_word =
                second_word;

            low_word =
                first_word;
        }

        return static_cast<std::uint32_t>(
            static_cast<std::uint32_t>(
                high_word
                )
            << 16U
            | static_cast<std::uint32_t>(
                low_word
                )
            );
    }
}