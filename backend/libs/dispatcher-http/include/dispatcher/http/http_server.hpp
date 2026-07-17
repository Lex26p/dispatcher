#pragma once

#include <dispatcher/api/transport_router.hpp>
#include <dispatcher/http/http_server_options.hpp>
#include <dispatcher/http/http_server_result.hpp>
#include <dispatcher/http/http_server_status.hpp>

#include <atomic>
#include <string>

namespace dispatcher::http
{
    class HttpServer
    {
    public:
        HttpServer(
            HttpServerOptions options,
            const dispatcher::api::TransportRouter& router
        );

        HttpServer(const HttpServer&) = delete;

        HttpServer& operator=(const HttpServer&) = delete;

        [[nodiscard]] const HttpServerOptions& options() const noexcept;

        [[nodiscard]] const dispatcher::api::TransportRouter& router()
            const noexcept;

        [[nodiscard]] HttpServerStatus status() const noexcept;

        [[nodiscard]] bool running() const noexcept;

        [[nodiscard]] bool stopped() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] bool stop_requested() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] std::string endpoint() const;

        [[nodiscard]] HttpServerResult run();

        void request_stop() noexcept;

    private:
        HttpServerOptions options_;
        const dispatcher::api::TransportRouter* router_{ nullptr };
        std::atomic_bool stop_requested_{ false };
        HttpServerStatus status_{ HttpServerStatus::Stopped };
    };
}