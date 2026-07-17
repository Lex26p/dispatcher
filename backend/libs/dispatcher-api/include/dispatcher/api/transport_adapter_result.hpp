#pragma once

#include <dispatcher/api/transport_adapter_status.hpp>
#include <dispatcher/api/transport_protocol.hpp>

#include <string>

namespace dispatcher::api
{
    class TransportAdapterResult
    {
    public:
        [[nodiscard]] static TransportAdapterResult success(
            TransportProtocol protocol,
            TransportAdapterStatus status,
            std::string message = {}
        );

        [[nodiscard]] static TransportAdapterResult failure(
            TransportProtocol protocol,
            TransportAdapterStatus status,
            std::string message = {},
            std::string detail = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] TransportProtocol protocol() const noexcept;

        [[nodiscard]] TransportAdapterStatus status() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& detail() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_detail() const noexcept;

    private:
        TransportAdapterResult(
            bool ok,
            TransportProtocol protocol,
            TransportAdapterStatus status,
            std::string message,
            std::string detail
        );

        bool ok_{ false };
        TransportProtocol protocol_{ TransportProtocol::Unknown };
        TransportAdapterStatus status_{ TransportAdapterStatus::Failed };
        std::string message_;
        std::string detail_;
    };
}