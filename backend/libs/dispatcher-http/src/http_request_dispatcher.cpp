#include <dispatcher/http/http_request_dispatcher.hpp>

#include <dispatcher/http/http_cors_headers.hpp>
#include <dispatcher/http/http_request_mapper.hpp>
#include <dispatcher/http/http_response_mapper.hpp>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <utility>

namespace dispatcher::http
{
    HttpRequestDispatcher::BeastResponse HttpRequestDispatcher::dispatch(
        const BeastRequest& request,
        const dispatcher::api::TransportRouter& router,
        std::string remote_address
    )
    {
        if (is_preflight_request(request))
        {
            return preflight_response(
                request.version(),
                request.keep_alive()
            );
        }

        const auto transport_request =
            HttpRequestMapper::to_transport_request(
                request,
                std::move(remote_address)
            );

        const auto result =
            router.dispatch(
                transport_request
            );

        if (!result.ok())
        {
            return not_found_response(
                request.version(),
                request.keep_alive()
            );
        }

        return HttpResponseMapper::to_beast_response(
            result.response(),
            request.version(),
            request.keep_alive()
        );
    }

    bool HttpRequestDispatcher::is_preflight_request(
        const BeastRequest& request
    )
    {
        return request.method() == boost::beast::http::verb::options;
    }

    HttpRequestDispatcher::BeastResponse HttpRequestDispatcher::json_response(
        boost::beast::http::status status,
        std::string body,
        unsigned version,
        bool keep_alive
    )
    {
        BeastResponse response{
            status,
            version
        };

        response.set(
            boost::beast::http::field::server,
            "dispatcher-http"
        );

        response.set(
            boost::beast::http::field::content_type,
            "application/json"
        );

        HttpCorsHeaders::apply(
            response
        );

        response.keep_alive(
            keep_alive
        );

        response.body() = std::move(body);

        response.prepare_payload();

        return response;
    }

    HttpRequestDispatcher::BeastResponse
        HttpRequestDispatcher::preflight_response(
            unsigned version,
            bool keep_alive
        )
    {
        return json_response(
            boost::beast::http::status::no_content,
            {},
            version,
            keep_alive
        );
    }

    HttpRequestDispatcher::BeastResponse
        HttpRequestDispatcher::not_found_response(
            unsigned version,
            bool keep_alive
        )
    {
        return json_response(
            boost::beast::http::status::not_found,
            "{\"status\":\"not_found\"}",
            version,
            keep_alive
        );
    }

    HttpRequestDispatcher::BeastResponse
        HttpRequestDispatcher::internal_error_response(
            unsigned version,
            bool keep_alive
        )
    {
        return json_response(
            boost::beast::http::status::internal_server_error,
            "{\"status\":\"internal_error\"}",
            version,
            keep_alive
        );
    }
}