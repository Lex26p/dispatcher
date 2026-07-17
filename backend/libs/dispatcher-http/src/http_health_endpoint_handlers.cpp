#include <dispatcher/http/http_health_endpoint_handlers.hpp>

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/runtime/health_status.hpp>
#include <dispatcher/runtime/readiness_status.hpp>

#include <sstream>
#include <string>
#include <utility>

namespace dispatcher::http
{
    namespace
    {
        [[nodiscard]] std::string bool_json(
            bool value
        )
        {
            return value ? "true" : "false";
        }

        [[nodiscard]] dispatcher::api::TransportEndpoint make_endpoint(
            std::string name,
            std::string path,
            std::string description
        )
        {
            return dispatcher::api::TransportEndpoint(
                std::move(name),
                dispatcher::api::TransportMethod::Get,
                std::move(path),
                false,
                false,
                std::move(description)
            );
        }
    }

    dispatcher::api::TransportRouter HttpHealthEndpointHandlers::build_router(
        dispatcher::runtime::HealthSnapshot health
    )
    {
        dispatcher::api::TransportRouter router;

        [[maybe_unused]] const auto registered =
            register_handlers(
                router,
                std::move(health)
            );

        return router;
    }

    bool HttpHealthEndpointHandlers::register_handlers(
        dispatcher::api::TransportRouter& router,
        dispatcher::runtime::HealthSnapshot health
    )
    {
        const auto health_result =
            router.add_handler(
                dispatcher::api::TransportHandler(
                    make_endpoint(
                        "health",
                        "/health",
                        "HTTP health endpoint"
                    ),
                    [health](const dispatcher::api::TransportRequest&)
                    {
                        return health_response(health);
                    }
                )
            );

        const auto readiness_result =
            router.add_handler(
                dispatcher::api::TransportHandler(
                    make_endpoint(
                        "readiness",
                        "/ready",
                        "HTTP readiness endpoint"
                    ),
                    [health](const dispatcher::api::TransportRequest&)
                    {
                        return readiness_response(health);
                    }
                )
            );

        return health_result.ok()
            && readiness_result.ok();
    }

    dispatcher::api::TransportResponse
        HttpHealthEndpointHandlers::health_response(
            const dispatcher::runtime::HealthSnapshot& health
        )
    {
        return dispatcher::api::TransportResponse::success(
            health_json(health)
        );
    }

    dispatcher::api::TransportResponse
        HttpHealthEndpointHandlers::readiness_response(
            const dispatcher::runtime::HealthSnapshot& health
        )
    {
        return dispatcher::api::TransportResponse::success(
            readiness_json(health)
        );
    }

    std::string HttpHealthEndpointHandlers::health_json(
        const dispatcher::runtime::HealthSnapshot& health
    )
    {
        std::ostringstream stream;

        stream
            << "{"
            << "\"status\":\""
            << dispatcher::runtime::to_string(health.overall_status())
            << "\","
            << "\"ready\":"
            << bool_json(health.ready())
            << ","
            << "\"check_count\":"
            << health.check_count()
            << ","
            << "\"healthy_count\":"
            << health.healthy_count()
            << ","
            << "\"degraded_count\":"
            << health.degraded_count()
            << ","
            << "\"unhealthy_count\":"
            << health.unhealthy_count()
            << ","
            << "\"invalid_count\":"
            << health.invalid_count()
            << "}";

        return stream.str();
    }

    std::string HttpHealthEndpointHandlers::readiness_json(
        const dispatcher::runtime::HealthSnapshot& health
    )
    {
        std::ostringstream stream;

        stream
            << "{"
            << "\"status\":\""
            << dispatcher::runtime::to_string(health.readiness_status())
            << "\","
            << "\"ready\":"
            << bool_json(health.ready())
            << ","
            << "\"readiness_blockers\":"
            << bool_json(health.has_readiness_blockers())
            << ","
            << "\"invalid_checks\":"
            << bool_json(health.has_invalid_checks())
            << ","
            << "\"check_count\":"
            << health.check_count()
            << "}";

        return stream.str();
    }
}