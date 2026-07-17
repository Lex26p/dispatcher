#pragma once

#include <dispatcher/api/transport_adapter.hpp>
#include <dispatcher/api/transport_adapter_result.hpp>
#include <dispatcher/api/transport_protocol.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace dispatcher::api
{
    class TransportAdapterRegistry
    {
    public:
        [[nodiscard]] TransportAdapterResult add(
            std::shared_ptr<TransportAdapter> adapter
        );

        [[nodiscard]] TransportAdapterResult remove(
            TransportProtocol protocol
        );

        [[nodiscard]] std::shared_ptr<TransportAdapter> find(
            TransportProtocol protocol
        ) const;

        [[nodiscard]] bool contains(
            TransportProtocol protocol
        ) const;

        [[nodiscard]] std::vector<std::shared_ptr<TransportAdapter>>
            adapters() const;

        [[nodiscard]] std::vector<std::shared_ptr<TransportAdapter>>
            running_adapters() const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void set_router_for_all(
            std::shared_ptr<TransportRouter> router
        );

        void clear() noexcept;

    private:
        std::vector<std::shared_ptr<TransportAdapter>> adapters_;
    };
}