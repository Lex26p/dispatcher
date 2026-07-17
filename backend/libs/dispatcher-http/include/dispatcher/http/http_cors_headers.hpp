#pragma once

namespace dispatcher::http
{
    class HttpCorsHeaders
    {
    public:
        template <typename Response>
        static void apply(
            Response& response
        )
        {
            response.set(
                "Access-Control-Allow-Origin",
                "*"
            );

            response.set(
                "Access-Control-Allow-Methods",
                "GET, POST, DELETE, OPTIONS"
            );

            response.set(
                "Access-Control-Allow-Headers",
                "Content-Type, Authorization, X-Correlation-Id, X-Request-Id"
            );

            response.set(
                "Access-Control-Max-Age",
                "600"
            );
        }
    };
}