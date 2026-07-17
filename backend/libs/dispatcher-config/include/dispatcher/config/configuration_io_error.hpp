#pragma once

#include <dispatcher/config/configuration_io_status.hpp>

#include <string>

namespace dispatcher::config
{
    struct ConfigurationIoError
    {
        ConfigurationIoStatus status{ ConfigurationIoStatus::UnknownError };

        std::string operation;
        std::string resource;
        std::string field;
        std::string message;

        [[nodiscard]] bool empty() const noexcept
        {
            return is_success(status)
                && operation.empty()
                && resource.empty()
                && field.empty()
                && message.empty();
        }

        [[nodiscard]] bool has_operation() const noexcept
        {
            return !operation.empty();
        }

        [[nodiscard]] bool has_resource() const noexcept
        {
            return !resource.empty();
        }

        [[nodiscard]] bool has_field() const noexcept
        {
            return !field.empty();
        }

        [[nodiscard]] bool has_message() const noexcept
        {
            return !message.empty();
        }
    };
}