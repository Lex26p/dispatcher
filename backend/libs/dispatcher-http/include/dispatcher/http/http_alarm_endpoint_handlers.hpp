#pragma once

#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_router.hpp>

#include <string>

namespace dispatcher::http
{
    class HttpAlarmEndpointHandlers
    {
    public:
        [[nodiscard]] static bool register_handlers(
            dispatcher::api::TransportRouter& router
        );

        [[nodiscard]] static dispatcher::api::TransportResponse
            alarms_response();

        [[nodiscard]] static std::string alarms_json();
    };
}