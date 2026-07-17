#include <dispatcher/http/http_alarm_endpoint_handlers.hpp>

#include <dispatcher/api/transport_endpoint.hpp>
#include <dispatcher/api/transport_handler.hpp>
#include <dispatcher/api/transport_method.hpp>
#include <dispatcher/api/transport_request.hpp>
#include <dispatcher/api/transport_response.hpp>
#include <dispatcher/http/http_alarm_contract.hpp>

#include <sstream>
#include <string>

namespace dispatcher::http
{
    namespace
    {
        [[nodiscard]] dispatcher::api::TransportEndpoint make_alarms_endpoint()
        {
            return dispatcher::api::TransportEndpoint(
                "alarms",
                dispatcher::api::TransportMethod::Get,
                "/api/v1/alarms",
                false,
                false,
                "HTTP alarms endpoint"
            );
        }
    }

    bool HttpAlarmEndpointHandlers::register_handlers(
        dispatcher::api::TransportRouter& router
    )
    {
        const auto result =
            router.add_handler(
                dispatcher::api::TransportHandler(
                    make_alarms_endpoint(),
                    [](const dispatcher::api::TransportRequest&)
                    {
                        return alarms_response();
                    }
                )
            );

        return result.ok();
    }

    dispatcher::api::TransportResponse
        HttpAlarmEndpointHandlers::alarms_response()
    {
        return dispatcher::api::TransportResponse::success(
            dispatcher::http::HttpAlarmContract::default_json()
        );
    }

    std::string HttpAlarmEndpointHandlers::alarms_json()
    {
        std::ostringstream stream;

        stream
            << "{"
            << "\"status\":\"available\","
            << "\"endpoint\":\"alarms\","
            << "\"path\":\"/api/v1/alarms\","
            << "\"method\":\"GET\","
            << "\"source\":\"dispatcher-http\","
            << "\"items\":[]"
            << "}";

        return stream.str();
    }
}