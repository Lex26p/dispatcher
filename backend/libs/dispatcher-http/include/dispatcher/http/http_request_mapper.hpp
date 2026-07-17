#pragma once

#include <dispatcher/api/transport_request.hpp>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string>

namespace dispatcher::http
{
    class HttpRequestMapper
    {
    public:
        using BeastRequest =
            boost::beast::http::request<boost::beast::http::string_body>;

        [[nodiscard]] static dispatcher::api::TransportRequest
            to_transport_request(
                const BeastRequest& request,
                std::string remote_address = {}
            );

        [[nodiscard]] static std::string method_string(
            const BeastRequest& request
        );

        [[nodiscard]] static std::string path_from_target(
            const BeastRequest& request
        );

        [[nodiscard]] static std::string correlation_id(
            const BeastRequest& request
        );

        [[nodiscard]] static std::string header_value(
            const BeastRequest& request,
            const std::string& header_name
        );
    };
}