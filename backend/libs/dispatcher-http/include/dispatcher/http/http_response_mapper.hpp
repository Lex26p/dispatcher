#pragma once

#include <dispatcher/api/transport_response.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace dispatcher::http
{
    class HttpResponseMapper
    {
    public:
        using BeastResponse =
            boost::beast::http::response<boost::beast::http::string_body>;

        [[nodiscard]] static BeastResponse to_beast_response(
            const dispatcher::api::TransportResponse& response,
            unsigned version = 11,
            bool keep_alive = false
        );
    };
}