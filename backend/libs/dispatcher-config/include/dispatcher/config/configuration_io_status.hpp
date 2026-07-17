#pragma once

namespace dispatcher::config
{
    enum class ConfigurationIoStatus
    {
        Success,

        UnsupportedFormat,
        ValidationError,
        ParseError,
        SerializationError,

        NotFound,
        Conflict,
        IoError,

        UnknownError
    };

    [[nodiscard]] const char* to_string(
        ConfigurationIoStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(ConfigurationIoStatus status) noexcept;

    [[nodiscard]] bool is_failure(ConfigurationIoStatus status) noexcept;
}