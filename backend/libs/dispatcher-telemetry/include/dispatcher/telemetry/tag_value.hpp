#pragma once

#include <dispatcher/domain/data_type.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace dispatcher::telemetry
{
    class TagValue
    {
    public:
        using Variant = std::variant<
            bool,
            std::int32_t,
            std::int64_t,
            float,
            double,
            std::string
        >;

        TagValue() = default;

        TagValue(bool value)
            : value_(value)
        {
        }

        TagValue(std::int32_t value)
            : value_(value)
        {
        }

        TagValue(std::int64_t value)
            : value_(value)
        {
        }

        TagValue(float value)
            : value_(value)
        {
        }

        TagValue(double value)
            : value_(value)
        {
        }

        TagValue(std::string value)
            : value_(std::move(value))
        {
        }

        TagValue(const char* value)
            : value_(std::string(value))
        {
        }

        [[nodiscard]] const Variant& raw() const noexcept
        {
            return value_;
        }

        [[nodiscard]] dispatcher::domain::DataType type() const noexcept
        {
            using dispatcher::domain::DataType;

            if (std::holds_alternative<bool>(value_))
            {
                return DataType::Boolean;
            }

            if (std::holds_alternative<std::int32_t>(value_))
            {
                return DataType::Int32;
            }

            if (std::holds_alternative<std::int64_t>(value_))
            {
                return DataType::Int64;
            }

            if (std::holds_alternative<float>(value_))
            {
                return DataType::Float32;
            }

            if (std::holds_alternative<double>(value_))
            {
                return DataType::Float64;
            }

            return DataType::String;
        }

        template <typename T>
        [[nodiscard]] const T& as() const
        {
            return std::get<T>(value_);
        }

    private:
        Variant value_{ std::int32_t{0} };
    };
}