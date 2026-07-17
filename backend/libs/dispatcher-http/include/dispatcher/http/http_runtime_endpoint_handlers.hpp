#pragma once

#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/api/transport_router.hpp>

#include <string>

namespace dispatcher::http
{
    class HttpRuntimeEndpointHandlers
    {
    public:
        [[nodiscard]] static bool register_handlers(
            dispatcher::api::TransportRouter& router
        );

        [[nodiscard]] static dispatcher::api::TransportResponse
            runtime_response();

        [[nodiscard]] static std::string runtime_json();
    };
}