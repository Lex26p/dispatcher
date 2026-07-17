#include <dispatcher/api/transport_endpoint_registry.hpp>

#include <algorithm>
#include <utility>

namespace dispatcher::api
{
    TransportEndpointResult TransportEndpointRegistry::add(
        TransportEndpoint endpoint
    )
    {
        if (!endpoint.valid())
        {
            return TransportEndpointResult::failure(
                TransportEndpointStatus::InvalidEndpoint,
                "endpoint is invalid",
                "endpoint",
                endpoint.key()
            );
        }

        const auto endpoint_key = endpoint.key();

        if (endpoints_by_key_.contains(endpoint_key))
        {
            return TransportEndpointResult::failure(
                TransportEndpointStatus::DuplicateEndpoint,
                "endpoint is already registered",
                "endpoint",
                endpoint_key
            );
        }

        endpoints_by_key_.emplace(
            endpoint_key,
            endpoint
        );

        return TransportEndpointResult::success(
            TransportEndpointStatus::Registered,
            std::move(endpoint),
            "endpoint registered"
        );
    }

    TransportEndpointResult TransportEndpointRegistry::remove(
        TransportMethod method,
        const std::string& path
    )
    {
        const auto endpoint_key =
            TransportEndpoint::make_key(
                method,
                path
            );

        auto iterator = endpoints_by_key_.find(endpoint_key);

        if (iterator == endpoints_by_key_.end())
        {
            return TransportEndpointResult::failure(
                TransportEndpointStatus::NotFound,
                "endpoint not found",
                "endpoint",
                endpoint_key
            );
        }

        auto endpoint = iterator->second;

        endpoints_by_key_.erase(iterator);

        return TransportEndpointResult::success(
            TransportEndpointStatus::Removed,
            std::move(endpoint),
            "endpoint removed"
        );
    }

    TransportEndpointResult TransportEndpointRegistry::resolve(
        TransportMethod method,
        const std::string& path
    ) const
    {
        auto endpoint = find(
            method,
            path
        );

        if (!endpoint.has_value())
        {
            return TransportEndpointResult::failure(
                TransportEndpointStatus::NotFound,
                "endpoint not found",
                "endpoint",
                TransportEndpoint::make_key(method, path)
            );
        }

        return TransportEndpointResult::success(
            TransportEndpointStatus::Found,
            std::move(endpoint.value()),
            "endpoint found"
        );
    }

    std::optional<TransportEndpoint> TransportEndpointRegistry::find(
        TransportMethod method,
        const std::string& path
    ) const
    {
        const auto endpoint_key =
            TransportEndpoint::make_key(
                method,
                path
            );

        const auto iterator = endpoints_by_key_.find(endpoint_key);

        if (iterator == endpoints_by_key_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    bool TransportEndpointRegistry::contains(
        TransportMethod method,
        const std::string& path
    ) const
    {
        return endpoints_by_key_.contains(
            TransportEndpoint::make_key(
                method,
                path
            )
        );
    }

    std::vector<TransportEndpoint> TransportEndpointRegistry::endpoints() const
    {
        std::vector<TransportEndpoint> result;

        result.reserve(endpoints_by_key_.size());

        for (const auto& [key, endpoint] : endpoints_by_key_)
        {
            result.push_back(endpoint);
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const TransportEndpoint& left, const TransportEndpoint& right)
            {
                if (left.path() == right.path())
                {
                    return to_string(left.method())
                        < std::string{ to_string(right.method()) };
                }

                return left.path() < right.path();
            }
        );

        return result;
    }

    std::size_t TransportEndpointRegistry::size() const noexcept
    {
        return endpoints_by_key_.size();
    }

    bool TransportEndpointRegistry::empty() const noexcept
    {
        return endpoints_by_key_.empty();
    }

    void TransportEndpointRegistry::clear() noexcept
    {
        endpoints_by_key_.clear();
    }
}