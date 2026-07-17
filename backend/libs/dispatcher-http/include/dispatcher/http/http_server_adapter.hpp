#pragma once

#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/http/http_server_options.hpp>
#include <dispatcher/http/http_server_result.hpp>
#include <dispatcher/http/http_server_status.hpp>

namespace dispatcher::http
{
    class HttpServerAdapter
    {
    public:
        explicit HttpServerAdapter(
            HttpServerOptions options = {}
        );

        HttpServerAdapter(
            HttpServerOptions options,
            const dispatcher::api::TransportRouter* router
        );

        [[nodiscard]] const HttpServerOptions& options() const noexcept;

        [[nodiscard]] const dispatcher::api::TransportRouter* router()
            const noexcept;

        [[nodiscard]] bool has_router() const noexcept;

        [[nodiscard]] HttpServerStatus status() const noexcept;

        [[nodiscard]] bool running() const noexcept;

        [[nodiscard]] bool stopped() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] bool can_start() const noexcept;

        [[nodiscard]] bool can_stop() const noexcept;

        void set_router(
            const dispatcher::api::TransportRouter* router
        ) noexcept;

        [[nodiscard]] HttpServerResult start();

        [[nodiscard]] HttpServerResult stop();

        [[nodiscard]] HttpServerResult mark_failed(
            std::string message
        );

    private:
        HttpServerOptions options_;
        const dispatcher::api::TransportRouter* router_{ nullptr };
        HttpServerStatus status_{ HttpServerStatus::Stopped };
    };
}