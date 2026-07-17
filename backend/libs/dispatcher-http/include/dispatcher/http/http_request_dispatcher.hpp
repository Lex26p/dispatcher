#pragma once

#include <dispatcher/api/transport_router.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string>

namespace dispatcher::http
{
    class HttpRequestDispatcher
    {
    public:
        using BeastRequest =
            boost::beast::http::request<boost::beast::http::string_body>;

        using BeastResponse =
            boost::beast::http::response<boost::beast::http::string_body>;

        [[nodiscard]] static BeastResponse dispatch(
            const BeastRequest& request,
            const dispatcher::api::TransportRouter& router,
            std::string remote_address = {}
        );

        [[nodiscard]] static bool is_preflight_request(
            const BeastRequest& request
        );

        [[nodiscard]] static BeastResponse json_response(
            boost::beast::http::status status,
            std::string body,
            unsigned version,
            bool keep_alive
        );

        [[nodiscard]] static BeastResponse preflight_response(
            unsigned version,
            bool keep_alive
        );

        [[nodiscard]] static BeastResponse not_found_response(
            unsigned version,
            bool keep_alive
        );

        [[nodiscard]] static BeastResponse internal_error_response(
            unsigned version,
            bool keep_alive
        );
    };
}