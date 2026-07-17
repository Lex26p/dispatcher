#pragma once

#include <string_view>

namespace dispatcher::config
{
    enum class ConfigurationFormat
    {
        Json,
        Unknown
    };

    [[nodiscard]] const char* to_string(ConfigurationFormat format) noexcept;

    [[nodiscard]] ConfigurationFormat parse_configuration_format(
        std::string_view value
    ) noexcept;

    [[nodiscard]] bool is_known_configuration_format(
        ConfigurationFormat format
    ) noexcept;
}