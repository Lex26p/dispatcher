#include <dispatcher/http/http_server_adapter.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <utility>

namespace dispatcher::http
{
    namespace
    {
        [[nodiscard]] bool boost_beast_dependency_available()
        {
            boost::asio::io_context io_context;
            boost::beast::flat_buffer buffer;

            return !io_context.stopped()
                && buffer.size() == 0;
        }
    }

    HttpServerAdapter::HttpServerAdapter(
        HttpServerOptions options
    )
        : options_(std::move(options))
    {
    }

    HttpServerAdapter::HttpServerAdapter(
        HttpServerOptions options,
        const dispatcher::api::TransportRouter* router
    )
        : options_(std::move(options))
        , router_(router)
    {
    }

    const HttpServerOptions& HttpServerAdapter::options() const noexcept
    {
        return options_;
    }

    const dispatcher::api::TransportRouter* HttpServerAdapter::router()
        const noexcept
    {
        return router_;
    }

    bool HttpServerAdapter::has_router() const noexcept
    {
        return router_ != nullptr;
    }

    HttpServerStatus HttpServerAdapter::status() const noexcept
    {
        return status_;
    }

    bool HttpServerAdapter::running() const noexcept
    {
        return is_running(status_);
    }

    bool HttpServerAdapter::stopped() const noexcept
    {
        return is_stopped(status_);
    }

    bool HttpServerAdapter::failed() const noexcept
    {
        return is_failed(status_);
    }

    bool HttpServerAdapter::can_start() const noexcept
    {
        return accepts_start(status_);
    }

    bool HttpServerAdapter::can_stop() const noexcept
    {
        return accepts_stop(status_);
    }

    void HttpServerAdapter::set_router(
        const dispatcher::api::TransportRouter* router
    ) noexcept
    {
        router_ = router;
    }

    HttpServerResult HttpServerAdapter::start()
    {
        if (!can_start())
        {
            return HttpServerResult::failure(
                status_,
                "http server cannot be started from current state"
            );
        }

        if (!options_.valid())
        {
            status_ = HttpServerStatus::Failed;

            return HttpServerResult::failure(
                status_,
                "http server options are invalid"
            );
        }

        if (!has_router())
        {
            status_ = HttpServerStatus::Failed;

            return HttpServerResult::failure(
                status_,
                "http server requires a transport router"
            );
        }

        if (!boost_beast_dependency_available())
        {
            status_ = HttpServerStatus::Failed;

            return HttpServerResult::failure(
                status_,
                "boost beast dependency is unavailable"
            );
        }

        status_ = HttpServerStatus::Running;

        return HttpServerResult::success(
            status_,
            "http server adapter started"
        );
    }

    HttpServerResult HttpServerAdapter::stop()
    {
        if (!can_stop())
        {
            return HttpServerResult::failure(
                status_,
                "http server cannot be stopped from current state"
            );
        }

        status_ = HttpServerStatus::Stopped;

        return HttpServerResult::success(
            status_,
            "http server adapter stopped"
        );
    }

    HttpServerResult HttpServerAdapter::mark_failed(
        std::string message
    )
    {
        status_ = HttpServerStatus::Failed;

        return HttpServerResult::failure(
            status_,
            std::move(message)
        );
    }
}