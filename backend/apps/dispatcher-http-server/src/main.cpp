#include <dispatcher/http/http_endpoint_router.hpp>
#include <dispatcher/http/http_server.hpp>
#include <dispatcher/http/http_server_options.hpp>
#include <dispatcher/http/http_server_result.hpp>
#include <dispatcher/runtime/health_check_result.hpp>
#include <dispatcher/runtime/health_snapshot.hpp>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
    dispatcher::runtime::HealthSnapshot make_health_snapshot()
    {
        dispatcher::runtime::HealthSnapshot snapshot;

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "runtime",
                "runtime",
                "runtime initialized"
            )
        );

        snapshot.add_check(
            dispatcher::runtime::HealthCheckResult::healthy(
                "http",
                "http",
                "http server initialized"
            )
        );

        return snapshot;
    }

    std::uint16_t parse_port(
        const char* value,
        std::uint16_t fallback
    )
    {
        if (value == nullptr)
        {
            return fallback;
        }

        try
        {
            const auto parsed =
                std::stoul(
                    value
                );

            if (parsed == 0
                || parsed > std::numeric_limits<std::uint16_t>::max())
            {
                return fallback;
            }

            return static_cast<std::uint16_t>(
                parsed
                );
        }
        catch (...)
        {
            return fallback;
        }
    }
}

int main(
    int argc,
    char** argv
)
{
    const std::string bind_address =
        argc > 1
        ? argv[1]
        : "127.0.0.1";

    const auto port =
        argc > 2
        ? parse_port(
            argv[2],
            8080
        )
        : static_cast<std::uint16_t>(8080);

    auto router =
        dispatcher::http::HttpEndpointRouter::build_router(
            make_health_snapshot()
        );

    dispatcher::http::HttpServer server(
        dispatcher::http::HttpServerOptions(
            bind_address,
            port
        ),
        router
    );

    std::cout
        << "dispatcher-http-server listening on http://"
        << server.endpoint()
        << '\n';

    std::cout
        << "Available endpoints:\n"
        << "  GET /health\n"
        << "  GET /ready\n"
        << "  GET /api/v1/runtime\n"
        << "  GET /api/v1/alarms\n"
        << "Press Ctrl+C to stop.\n";

    const auto result =
        server.run();

    if (!result.ok())
    {
        std::cerr
            << "dispatcher-http-server failed: "
            << result.message()
            << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}