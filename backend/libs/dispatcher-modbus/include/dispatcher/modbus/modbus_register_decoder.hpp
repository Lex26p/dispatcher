#pragma once

#include <cstdint>
#include <vector>

namespace dispatcher::modbus
{
    enum class ModbusRegisterValueType
    {
        uint16,
        int16,
        uint32,
        int32,
        float32
    };

    enum class ModbusWordOrder
    {
        high_word_first,
        low_word_first
    };

    struct ModbusDecodedRegisterValue
    {
        ModbusRegisterValueType value_type{
            ModbusRegisterValueType::uint16
        };

        double numeric_value{ 0.0 };
    };

    class ModbusRegisterDecoder final
    {
    public:
        [[nodiscard]] static std::uint16_t decode_uint16(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset = 0
        );

        [[nodiscard]] static std::int16_t decode_int16(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset = 0
        );

        [[nodiscard]] static std::uint32_t decode_uint32(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset = 0,
            ModbusWordOrder word_order = ModbusWordOrder::high_word_first
        );

        [[nodiscard]] static std::int32_t decode_int32(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset = 0,
            ModbusWordOrder word_order = ModbusWordOrder::high_word_first
        );

        [[nodiscard]] static float decode_float32(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset = 0,
            ModbusWordOrder word_order = ModbusWordOrder::high_word_first
        );

        [[nodiscard]] static ModbusDecodedRegisterValue decode_numeric(
            const std::vector<std::uint16_t>& registers,
            ModbusRegisterValueType value_type,
            std::size_t offset = 0,
            ModbusWordOrder word_order = ModbusWordOrder::high_word_first
        );

    private:
        static void require_registers(
            const std::vector<std::uint16_t>& registers,
            std::size_t offset,
            std::size_t required_count
        );

        [[nodiscard]] static std::uint32_t compose_u32(
            std::uint16_t first_word,
            std::uint16_t second_word,
            ModbusWordOrder word_order
        );
    };
}