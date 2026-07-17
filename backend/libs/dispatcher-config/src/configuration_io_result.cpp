#include <dispatcher/config/configuration_io_result.hpp>

#include <utility>

namespace dispatcher::config
{
    ConfigurationIoResult ConfigurationIoResult::success()
    {
        return ConfigurationIoResult(
            ConfigurationIoError{
                .status = ConfigurationIoStatus::Success
            }
        );
    }

    ConfigurationIoResult ConfigurationIoResult::failure(
        ConfigurationIoStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        if (is_success(status))
        {
            status = ConfigurationIoStatus::UnknownError;
        }

        return ConfigurationIoResult(
            ConfigurationIoError{
                .status = status,
                .operation = std::move(operation),
                .resource = std::move(resource),
                .field = std::move(field),
                .message = std::move(message)
            }
        );
    }

    bool ConfigurationIoResult::ok() const noexcept
    {
        return is_success(error_.status);
    }

    bool ConfigurationIoResult::failed() const noexcept
    {
        return !ok();
    }

    ConfigurationIoStatus ConfigurationIoResult::status() const noexcept
    {
        return error_.status;
    }

    const ConfigurationIoError& ConfigurationIoResult::error() const noexcept
    {
        return error_;
    }

    const std::string& ConfigurationIoResult::operation() const noexcept
    {
        return error_.operation;
    }

    const std::string& ConfigurationIoResult::resource() const noexcept
    {
        return error_.resource;
    }

    const std::string& ConfigurationIoResult::field() const noexcept
    {
        return error_.field;
    }

    const std::string& ConfigurationIoResult::message() const noexcept
    {
        return error_.message;
    }

    ConfigurationIoResult::ConfigurationIoResult(
        ConfigurationIoError error
    )
        : error_(std::move(error))
    {
    }
}