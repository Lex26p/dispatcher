#include <dispatcher/http/http_endpoint_router.hpp>

#include <dispatcher/http/http_alarm_endpoint_handlers.hpp>
#include <dispatcher/http/http_health_endpoint_handlers.hpp>
#include <dispatcher/http/http_runtime_endpoint_handlers.hpp>

#include <utility>

namespace dispatcher::http
{
    dispatcher::api::TransportRouter HttpEndpointRouter::build_router(
        dispatcher::runtime::HealthSnapshot health
    )
    {
        dispatcher::api::TransportRouter router;

        [[maybe_unused]] const auto registered =
            register_all(
                router,
                std::move(health)
            );

        return router;
    }

    bool HttpEndpointRouter::register_all(
        dispatcher::api::TransportRouter& router,
        dispatcher::runtime::HealthSnapshot health
    )
    {
        const auto health_registered =
            HttpHealthEndpointHandlers::register_handlers(
                router,
                std::move(health)
            );

        const auto runtime_registered =
            HttpRuntimeEndpointHandlers::register_handlers(
                router
            );

        const auto alarms_registered =
            HttpAlarmEndpointHandlers::register_handlers(
                router
            );

        return health_registered
            && runtime_registered
            && alarms_registered;
    }
}