#include <dispatcher/http/http_server.hpp>

#include <dispatcher/http/http_request_dispatcher.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>

#include <exception>
#include <string>
#include <utility>

namespace dispatcher::http
{
    namespace
    {
        namespace asio = boost::asio;
        namespace beast = boost::beast;
        namespace http = boost::beast::http;

        using Tcp = asio::ip::tcp;

        [[nodiscard]] std::string socket_remote_address(
            const Tcp::socket& socket
        )
        {
            boost::system::error_code error;

            const auto endpoint =
                socket.remote_endpoint(
                    error
                );

            if (error)
            {
                return {};
            }

            return endpoint.address().to_string();
        }

        void handle_session(
            Tcp::socket socket,
            const dispatcher::api::TransportRouter& router,
            std::uint64_t request_body_limit_bytes
        )
        {
            beast::flat_buffer buffer;

            http::request_parser<http::string_body> parser;

            parser.body_limit(
                request_body_limit_bytes
            );

            boost::system::error_code error;

            http::read(
                socket,
                buffer,
                parser,
                error
            );

            if (error)
            {
                return;
            }

            auto request =
                parser.release();

            const auto remote_address =
                socket_remote_address(
                    socket
                );

            auto response =
                HttpRequestDispatcher::dispatch(
                    request,
                    router,
                    remote_address
                );

            http::write(
                socket,
                response,
                error
            );

            boost::system::error_code shutdown_error;

            socket.shutdown(
                Tcp::socket::shutdown_send,
                shutdown_error
            );
        }
    }

    HttpServer::HttpServer(
        HttpServerOptions options,
        const dispatcher::api::TransportRouter& router
    )
        : options_(std::move(options))
        , router_(&router)
    {
    }

    const HttpServerOptions& HttpServer::options() const noexcept
    {
        return options_;
    }

    const dispatcher::api::TransportRouter& HttpServer::router()
        const noexcept
    {
        return *router_;
    }

    HttpServerStatus HttpServer::status() const noexcept
    {
        return status_;
    }

    bool HttpServer::running() const noexcept
    {
        return is_running(status_);
    }

    bool HttpServer::stopped() const noexcept
    {
        return is_stopped(status_);
    }

    bool HttpServer::failed() const noexcept
    {
        return is_failed(status_);
    }

    bool HttpServer::stop_requested() const noexcept
    {
        return stop_requested_.load();
    }

    bool HttpServer::valid() const noexcept
    {
        return router_ != nullptr
            && options_.valid();
    }

    std::string HttpServer::endpoint() const
    {
        return options_.endpoint();
    }

    HttpServerResult HttpServer::run()
    {
        if (!valid())
        {
            status_ = HttpServerStatus::Failed;

            return HttpServerResult::failure(
                status_,
                "http server configuration is invalid"
            );
        }

        stop_requested_.store(
            false
        );

        status_ = HttpServerStatus::Starting;

        try
        {
            asio::io_context io_context;

            boost::system::error_code error;

            const auto address =
                asio::ip::make_address(
                    options_.bind_address(),
                    error
                );

            if (error)
            {
                status_ = HttpServerStatus::Failed;

                return HttpServerResult::failure(
                    status_,
                    "invalid bind address: " + options_.bind_address()
                );
            }

            const Tcp::endpoint endpoint{
                address,
                options_.port()
            };

            Tcp::acceptor acceptor{
                io_context
            };

            acceptor.open(
                endpoint.protocol(),
                error
            );

            if (error)
            {
                status_ = HttpServerStatus::Failed;

                return HttpServerResult::failure(
                    status_,
                    "failed to open acceptor"
                );
            }

            acceptor.set_option(
                asio::socket_base::reuse_address(
                    options_.reuse_address()
                ),
                error
            );

            if (error)
            {
                status_ = HttpServerStatus::Failed;

                return HttpServerResult::failure(
                    status_,
                    "failed to set reuse_address"
                );
            }

            acceptor.bind(
                endpoint,
                error
            );

            if (error)
            {
                status_ = HttpServerStatus::Failed;

                return HttpServerResult::failure(
                    status_,
                    "failed to bind endpoint " + options_.endpoint()
                );
            }

            acceptor.listen(
                asio::socket_base::max_listen_connections,
                error
            );

            if (error)
            {
                status_ = HttpServerStatus::Failed;

                return HttpServerResult::failure(
                    status_,
                    "failed to listen on endpoint " + options_.endpoint()
                );
            }

            status_ = HttpServerStatus::Running;

            while (!stop_requested())
            {
                Tcp::socket socket{
                    io_context
                };

                acceptor.accept(
                    socket,
                    error
                );

                if (error)
                {
                    if (stop_requested())
                    {
                        break;
                    }

                    status_ = HttpServerStatus::Failed;

                    return HttpServerResult::failure(
                        status_,
                        "failed to accept connection"
                    );
                }

                handle_session(
                    std::move(socket),
                    router(),
                    options_.request_body_limit_bytes()
                );
            }

            status_ = HttpServerStatus::Stopped;

            return HttpServerResult::success(
                status_,
                "http server stopped"
            );
        }
        catch (const std::exception& exception)
        {
            status_ = HttpServerStatus::Failed;

            return HttpServerResult::failure(
                status_,
                exception.what()
            );
        }
    }

    void HttpServer::request_stop() noexcept
    {
        stop_requested_.store(
            true
        );
    }
}