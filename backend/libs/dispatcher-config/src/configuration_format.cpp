#include <dispatcher/config/configuration_format.hpp>

namespace dispatcher::config
{
    const char* to_string(ConfigurationFormat format) noexcept
    {
        switch (format)
        {
        case ConfigurationFormat::Json:
            return "json";

        case ConfigurationFormat::Unknown:
            return "unknown";
        }

        return "unknown";
    }

    ConfigurationFormat parse_configuration_format(
        std::string_view value
    ) noexcept
    {
        if (
            value == "json"
            || value == "JSON"
            || value == "application/json"
            )
        {
            return ConfigurationFormat::Json;
        }

        return ConfigurationFormat::Unknown;
    }

    bool is_known_configuration_format(ConfigurationFormat format) noexcept
    {
        return format != ConfigurationFormat::Unknown;
    }
}