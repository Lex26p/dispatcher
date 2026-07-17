#include <dispatcher/http/http_response_mapper.hpp>

#include <dispatcher/http/http_cors_headers.hpp>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>

namespace dispatcher::http
{
    HttpResponseMapper::BeastResponse HttpResponseMapper::to_beast_response(
        const dispatcher::api::TransportResponse& response,
        unsigned version,
        bool keep_alive
    )
    {
        BeastResponse beast_response{
            static_cast<boost::beast::http::status>(
                response.http_status()
            ),
            version
        };

        beast_response.set(
            boost::beast::http::field::server,
            "dispatcher-http"
        );

        beast_response.set(
            boost::beast::http::field::content_type,
            "application/json"
        );

        HttpCorsHeaders::apply(
            beast_response
        );

        beast_response.keep_alive(
            keep_alive
        );

        beast_response.body() = response.body();

        beast_response.prepare_payload();

        return beast_response;
    }
}