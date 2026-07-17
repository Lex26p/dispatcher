#pragma once

#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

namespace dispatcher::http
{
    class HttpEndpointRouter
    {
    public:
        [[nodiscard]] static dispatcher::api::TransportRouter build_router(
            dispatcher::runtime::HealthSnapshot health
        );

        [[nodiscard]] static bool register_all(
            dispatcher::api::TransportRouter& router,
            dispatcher::runtime::HealthSnapshot health
        );
    };
}