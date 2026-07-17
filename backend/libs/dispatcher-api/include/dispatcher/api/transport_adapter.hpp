#pragma once

#include <dispatcher/api/transport_adapter_options.hpp>
#include <dispatcher/api/transport_adapter_result.hpp>
#include <dispatcher/api/transport_adapter_status.hpp>
#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_router.hpp>

#include <memory>
#include <string>

namespace dispatcher::api
{
    class TransportAdapter
    {
    public:
        virtual ~TransportAdapter();

        [[nodiscard]] virtual TransportProtocol protocol() const noexcept = 0;

        [[nodiscard]] virtual std::string name() const = 0;

        [[nodiscard]] virtual TransportAdapterStatus status()
            const noexcept = 0;

        [[nodiscard]] virtual const TransportAdapterOptions& options()
            const noexcept = 0;

        virtual void set_router(
            std::shared_ptr<TransportRouter> router
        ) = 0;

        [[nodiscard]] virtual bool has_router() const noexcept = 0;

        [[nodiscard]] virtual TransportAdapterResult start() = 0;

        [[nodiscard]] virtual TransportAdapterResult stop() = 0;

        [[nodiscard]] virtual TransportAdapterResult health() const = 0;

        [[nodiscard]] bool running() const noexcept;

        [[nodiscard]] bool stopped() const noexcept;

        [[nodiscard]] bool accepts_requests() const noexcept;
    };
}