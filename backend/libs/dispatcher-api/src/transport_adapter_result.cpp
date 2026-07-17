#include <dispatcher/api/transport_adapter_result.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportAdapterResult TransportAdapterResult::success(
        TransportProtocol protocol,
        TransportAdapterStatus status,
        std::string message
    )
    {
        if (is_failure(status))
        {
            status = TransportAdapterStatus::Running;
        }

        return TransportAdapterResult(
            true,
            protocol,
            status,
            std::move(message),
            {}
        );
    }

    TransportAdapterResult TransportAdapterResult::failure(
        TransportProtocol protocol,
        TransportAdapterStatus status,
        std::string message,
        std::string detail
    )
    {
        if (!is_failure(status))
        {
            status = TransportAdapterStatus::Failed;
        }

        return TransportAdapterResult(
            false,
            protocol,
            status,
            std::move(message),
            std::move(detail)
        );
    }

    bool TransportAdapterResult::ok() const noexcept
    {
        return ok_;
    }

    bool TransportAdapterResult::failed() const noexcept
    {
        return !ok_;
    }

    TransportProtocol TransportAdapterResult::protocol() const noexcept
    {
        return protocol_;
    }

    TransportAdapterStatus TransportAdapterResult::status() const noexcept
    {
        return status_;
    }

    const std::string& TransportAdapterResult::message() const noexcept
    {
        return message_;
    }

    const std::string& TransportAdapterResult::detail() const noexcept
    {
        return detail_;
    }

    bool TransportAdapterResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool TransportAdapterResult::has_detail() const noexcept
    {
        return !detail_.empty();
    }

    TransportAdapterResult::TransportAdapterResult(
        bool ok,
        TransportProtocol protocol,
        TransportAdapterStatus status,
        std::string message,
        std::string detail
    )
        : ok_(ok)
        , protocol_(protocol)
        , status_(status)
        , message_(std::move(message))
        , detail_(std::move(detail))
    {
    }
}