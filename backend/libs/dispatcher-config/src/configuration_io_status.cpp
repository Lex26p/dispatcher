#include <dispatcher/config/configuration_io_status.hpp>

namespace dispatcher::config
{
    const char* to_string(ConfigurationIoStatus status) noexcept
    {
        switch (status)
        {
        case ConfigurationIoStatus::Success:
            return "success";

        case ConfigurationIoStatus::UnsupportedFormat:
            return "unsupported_format";

        case ConfigurationIoStatus::ValidationError:
            return "validation_error";

        case ConfigurationIoStatus::ParseError:
            return "parse_error";

        case ConfigurationIoStatus::SerializationError:
            return "serialization_error";

        case ConfigurationIoStatus::NotFound:
            return "not_found";

        case ConfigurationIoStatus::Conflict:
            return "conflict";

        case ConfigurationIoStatus::IoError:
            return "io_error";

        case ConfigurationIoStatus::UnknownError:
            return "unknown_error";
        }

        return "unknown_error";
    }

    bool is_success(ConfigurationIoStatus status) noexcept
    {
        return status == ConfigurationIoStatus::Success;
    }

    bool is_failure(ConfigurationIoStatus status) noexcept
    {
        return !is_success(status);
    }
}