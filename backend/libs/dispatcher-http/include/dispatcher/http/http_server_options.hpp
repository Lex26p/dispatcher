#pragma once

#include <cstdint>
#include <string>

namespace dispatcher::http
{
    class HttpServerOptions
    {
    public:
        HttpServerOptions();

        HttpServerOptions(
            std::string bind_address,
            std::uint16_t port,
            std::uint16_t worker_threads = 1,
            std::uint64_t request_body_limit_bytes = 1024ULL * 1024ULL,
            bool reuse_address = true
        );

        [[nodiscard]] const std::string& bind_address() const noexcept;

        [[nodiscard]] std::uint16_t port() const noexcept;

        [[nodiscard]] std::uint16_t worker_threads() const noexcept;

        [[nodiscard]] std::uint64_t request_body_limit_bytes() const noexcept;

        [[nodiscard]] bool reuse_address() const noexcept;

        [[nodiscard]] bool has_bind_address() const noexcept;

        [[nodiscard]] bool has_valid_port() const noexcept;

        [[nodiscard]] bool has_worker_threads() const noexcept;

        [[nodiscard]] bool has_request_body_limit() const noexcept;

        [[nodiscard]] bool valid() const noexcept;

        [[nodiscard]] std::string endpoint() const;

    private:
        std::string bind_address_{ "127.0.0.1" };
        std::uint16_t port_{ 8080 };
        std::uint16_t worker_threads_{ 1 };
        std::uint64_t request_body_limit_bytes_{ 1024ULL * 1024ULL };
        bool reuse_address_{ true };
    };
}