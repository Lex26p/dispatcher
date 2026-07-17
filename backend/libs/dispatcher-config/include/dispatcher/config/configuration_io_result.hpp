#pragma once

#include <dispatcher/config/configuration_io_error.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <string>

namespace dispatcher::config
{
    class ConfigurationIoResult
    {
    public:
        [[nodiscard]] static ConfigurationIoResult success();

        [[nodiscard]] static ConfigurationIoResult failure(
            ConfigurationIoStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ConfigurationIoStatus status() const noexcept;

        [[nodiscard]] const ConfigurationIoError& error() const noexcept;

        [[nodiscard]] const std::string& operation() const noexcept;

        [[nodiscard]] const std::string& resource() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

    private:
        explicit ConfigurationIoResult(ConfigurationIoError error);

        ConfigurationIoError error_;
    };
}