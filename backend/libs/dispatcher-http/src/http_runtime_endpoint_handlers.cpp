#include <dispatcher/http/http_runtime_endpoint_handlers.hpp>

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/http/http_runtime_contract.hpp>

#include <sstream>
#include <string>
#include <utility>

namespace dispatcher::http
{
    namespace
    {
        [[nodiscard]] dispatcher::api::TransportEndpoint make_runtime_endpoint()
        {
            return dispatcher::api::TransportEndpoint(
                "runtime",
                dispatcher::api::TransportMethod::Get,
                "/api/v1/runtime",
                false,
                false,
                "HTTP runtime endpoint"
            );
        }
    }

    bool HttpRuntimeEndpointHandlers::register_handlers(
        dispatcher::api::TransportRouter& router
    )
    {
        const auto result =
            router.add_handler(
                dispatcher::api::TransportHandler(
                    make_runtime_endpoint(),
                    [](const dispatcher::api::TransportRequest&)
                    {
                        return runtime_response();
                    }
                )
            );

        return result.ok();
    }

    dispatcher::api::TransportResponse
        HttpRuntimeEndpointHandlers::runtime_response()
    {
        return dispatcher::api::TransportResponse::success(
            dispatcher::http::HttpRuntimeContract::default_json()
        );
    }

    std::string HttpRuntimeEndpointHandlers::runtime_json()
    {
        std::ostringstream stream;

        stream
            << "{"
            << "\"status\":\"available\","
            << "\"endpoint\":\"runtime\","
            << "\"path\":\"/api/v1/runtime\","
            << "\"method\":\"GET\","
            << "\"source\":\"dispatcher-http\""
            << "}";

        return stream.str();
    }
}