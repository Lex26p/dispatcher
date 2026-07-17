#include <dispatcher/http/http_server_options.hpp>

#include <utility>

namespace dispatcher::http
{
    HttpServerOptions::HttpServerOptions() = default;

    HttpServerOptions::HttpServerOptions(
        std::string bind_address,
        std::uint16_t port,
        std::uint16_t worker_threads,
        std::uint64_t request_body_limit_bytes,
        bool reuse_address
    )
        : bind_address_(std::move(bind_address))
        , port_(port)
        , worker_threads_(worker_threads)
        , request_body_limit_bytes_(request_body_limit_bytes)
        , reuse_address_(reuse_address)
    {
    }

    const std::string& HttpServerOptions::bind_address() const noexcept
    {
        return bind_address_;
    }

    std::uint16_t HttpServerOptions::port() const noexcept
    {
        return port_;
    }

    std::uint16_t HttpServerOptions::worker_threads() const noexcept
    {
        return worker_threads_;
    }

    std::uint64_t HttpServerOptions::request_body_limit_bytes()
        const noexcept
    {
        return request_body_limit_bytes_;
    }

    bool HttpServerOptions::reuse_address() const noexcept
    {
        return reuse_address_;
    }

    bool HttpServerOptions::has_bind_address() const noexcept
    {
        return !bind_address_.empty();
    }

    bool HttpServerOptions::has_valid_port() const noexcept
    {
        return port_ > 0;
    }

    bool HttpServerOptions::has_worker_threads() const noexcept
    {
        return worker_threads_ > 0;
    }

    bool HttpServerOptions::has_request_body_limit() const noexcept
    {
        return request_body_limit_bytes_ > 0;
    }

    bool HttpServerOptions::valid() const noexcept
    {
        return has_bind_address()
            && has_valid_port()
            && has_worker_threads()
            && has_request_body_limit();
    }

    std::string HttpServerOptions::endpoint() const
    {
        return bind_address_ + ":" + std::to_string(port_);
    }
}