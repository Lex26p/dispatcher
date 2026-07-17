#include <dispatcher/api/transport_adapter.hpp>

namespace dispatcher::api
{
    TransportAdapter::~TransportAdapter() = default;

    bool TransportAdapter::running() const noexcept
    {
        return is_running(status());
    }

    bool TransportAdapter::stopped() const noexcept
    {
        return is_stopped(status());
    }

    bool TransportAdapter::accepts_requests() const noexcept
    {
        return dispatcher::api::accepts_requests(status());
    }
}