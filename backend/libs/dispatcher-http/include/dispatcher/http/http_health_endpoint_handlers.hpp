#pragma once

#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

namespace dispatcher::http
{
    class HttpHealthEndpointHandlers
    {
    public:
        [[nodiscard]] static dispatcher::api::TransportRouter
            build_router(
                dispatcher::runtime::HealthSnapshot health
            );

        [[nodiscard]] static bool register_handlers(
            dispatcher::api::TransportRouter& router,
            dispatcher::runtime::HealthSnapshot health
        );

        [[nodiscard]] static dispatcher::api::TransportResponse
            health_response(
                const dispatcher::runtime::HealthSnapshot& health
            );

        [[nodiscard]] static dispatcher::api::TransportResponse
            readiness_response(
                const dispatcher::runtime::HealthSnapshot& health
            );

        [[nodiscard]] static std::string health_json(
            const dispatcher::runtime::HealthSnapshot& health
        );

        [[nodiscard]] static std::string readiness_json(
            const dispatcher::runtime::HealthSnapshot& health
        );
    };
}