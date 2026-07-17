#pragma once

#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace dispatcher::http
{
    class HttpJson
    {
    public:
        [[nodiscard]] static std::string escape(
            std::string_view value
        )
        {
            std::string escaped;
            escaped.reserve(
                value.size() + 8
            );

            for (const auto character : value)
            {
                switch (character)
                {
                case '"':
                    escaped += "\\\"";
                    break;

                case '\\':
                    escaped += "\\\\";
                    break;

                case '\b':
                    escaped += "\\b";
                    break;

                case '\f':
                    escaped += "\\f";
                    break;

                case '\n':
                    escaped += "\\n";
                    break;

                case '\r':
                    escaped += "\\r";
                    break;

                case '\t':
                    escaped += "\\t";
                    break;

                default:
                    if (static_cast<unsigned char>(character) < 0x20)
                    {
                        escaped += "\\u00";
                        escaped += hex_digit(
                            static_cast<unsigned char>(character) >> 4
                        );
                        escaped += hex_digit(
                            static_cast<unsigned char>(character) & 0x0F
                        );
                    }
                    else
                    {
                        escaped += character;
                    }

                    break;
                }
            }

            return escaped;
        }

        [[nodiscard]] static std::string quoted(
            std::string_view value
        )
        {
            return "\"" + escape(value) + "\"";
        }

        [[nodiscard]] static std::string string_field(
            std::string_view name,
            std::string_view value
        )
        {
            return quoted(name) + ":" + quoted(value);
        }

        [[nodiscard]] static std::string bool_field(
            std::string_view name,
            bool value
        )
        {
            return quoted(name) + ":" + bool_value(value);
        }

        [[nodiscard]] static std::string int_field(
            std::string_view name,
            std::int64_t value
        )
        {
            return quoted(name) + ":" + std::to_string(value);
        }

        [[nodiscard]] static std::string uint_field(
            std::string_view name,
            std::uint64_t value
        )
        {
            return quoted(name) + ":" + std::to_string(value);
        }

        [[nodiscard]] static std::string null_field(
            std::string_view name
        )
        {
            return quoted(name) + ":null";
        }

        [[nodiscard]] static std::string raw_field(
            std::string_view name,
            std::string_view raw_json_value
        )
        {
            return quoted(name) + ":" + std::string(raw_json_value);
        }

        [[nodiscard]] static std::string object(
            std::initializer_list<std::string> fields
        )
        {
            return object(
                std::vector<std::string>(
                    fields
                )
            );
        }

        [[nodiscard]] static std::string object(
            const std::vector<std::string>& fields
        )
        {
            std::string result = "{";

            for (std::size_t index = 0; index < fields.size(); ++index)
            {
                if (index > 0)
                {
                    result += ",";
                }

                result += fields[index];
            }

            result += "}";

            return result;
        }

        [[nodiscard]] static std::string array(
            std::initializer_list<std::string> values
        )
        {
            return array(
                std::vector<std::string>(
                    values
                )
            );
        }

        [[nodiscard]] static std::string array(
            const std::vector<std::string>& values
        )
        {
            std::string result = "[";

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (index > 0)
                {
                    result += ",";
                }

                result += values[index];
            }

            result += "]";

            return result;
        }

        [[nodiscard]] static std::string bool_value(
            bool value
        )
        {
            return value
                ? "true"
                : "false";
        }

    private:
        [[nodiscard]] static char hex_digit(
            unsigned char value
        )
        {
            constexpr char digits[] = "0123456789abcdef";

            return digits[value & 0x0F];
        }
    };
}