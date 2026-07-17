#include <dispatcher/api/transport_adapter_registry.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::api
{
    TransportAdapterResult TransportAdapterRegistry::add(
        std::shared_ptr<TransportAdapter> adapter
    )
    {
        if (!adapter)
        {
            return TransportAdapterResult::failure(
                TransportProtocol::Unknown,
                TransportAdapterStatus::Failed,
                "transport adapter is null",
                "adapter"
            );
        }

        if (!is_known_protocol(adapter->protocol()))
        {
            return TransportAdapterResult::failure(
                adapter->protocol(),
                TransportAdapterStatus::Failed,
                "transport adapter protocol is unknown",
                "protocol"
            );
        }

        if (contains(adapter->protocol()))
        {
            return TransportAdapterResult::failure(
                adapter->protocol(),
                TransportAdapterStatus::Failed,
                "transport adapter protocol is already registered",
                to_string(adapter->protocol())
            );
        }

        const auto protocol = adapter->protocol();

        adapters_.push_back(
            std::move(adapter)
        );

        return TransportAdapterResult::success(
            protocol,
            TransportAdapterStatus::Stopped,
            "transport adapter registered"
        );
    }

    TransportAdapterResult TransportAdapterRegistry::remove(
        TransportProtocol protocol
    )
    {
        const auto iterator =
            std::find_if(
                adapters_.begin(),
                adapters_.end(),
                [&](const std::shared_ptr<TransportAdapter>& adapter)
                {
                    return adapter
                        && adapter->protocol() == protocol;
                }
            );

        if (iterator == adapters_.end())
        {
            return TransportAdapterResult::failure(
                protocol,
                TransportAdapterStatus::Failed,
                "transport adapter not found",
                to_string(protocol)
            );
        }

        adapters_.erase(iterator);

        return TransportAdapterResult::success(
            protocol,
            TransportAdapterStatus::Stopped,
            "transport adapter removed"
        );
    }

    std::shared_ptr<TransportAdapter> TransportAdapterRegistry::find(
        TransportProtocol protocol
    ) const
    {
        const auto iterator =
            std::find_if(
                adapters_.begin(),
                adapters_.end(),
                [&](const std::shared_ptr<TransportAdapter>& adapter)
                {
                    return adapter
                        && adapter->protocol() == protocol;
                }
            );

        if (iterator == adapters_.end())
        {
            return {};
        }

        return *iterator;
    }

    bool TransportAdapterRegistry::contains(
        TransportProtocol protocol
    ) const
    {
        return find(protocol) != nullptr;
    }

    std::vector<std::shared_ptr<TransportAdapter>>
        TransportAdapterRegistry::adapters() const
    {
        auto result = adapters_;

        std::sort(
            result.begin(),
            result.end(),
            [](const std::shared_ptr<TransportAdapter>& left,
                const std::shared_ptr<TransportAdapter>& right)
            {
                if (!left)
                {
                    return false;
                }

                if (!right)
                {
                    return true;
                }

                return to_string(left->protocol())
                    < std::string{ to_string(right->protocol()) };
            }
        );

        return result;
    }

    std::vector<std::shared_ptr<TransportAdapter>>
        TransportAdapterRegistry::running_adapters() const
    {
        std::vector<std::shared_ptr<TransportAdapter>> result;

        for (const auto& adapter : adapters_)
        {
            if (adapter && adapter->running())
            {
                result.push_back(adapter);
            }
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const std::shared_ptr<TransportAdapter>& left,
                const std::shared_ptr<TransportAdapter>& right)
            {
                return to_string(left->protocol())
                    < std::string{ to_string(right->protocol()) };
            }
        );

        return result;
    }

    std::size_t TransportAdapterRegistry::size() const noexcept
    {
        return adapters_.size();
    }

    bool TransportAdapterRegistry::empty() const noexcept
    {
        return adapters_.empty();
    }

    void TransportAdapterRegistry::set_router_for_all(
        std::shared_ptr<TransportRouter> router
    )
    {
        for (const auto& adapter : adapters_)
        {
            if (adapter)
            {
                adapter->set_router(router);
            }
        }
    }

    void TransportAdapterRegistry::clear() noexcept
    {
        adapters_.clear();
    }
}