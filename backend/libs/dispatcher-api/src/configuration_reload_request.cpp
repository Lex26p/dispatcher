#include <dispatcher/api/configuration_reload_request.hpp>

#include <utility>

namespace dispatcher::api
{
    ConfigurationReloadRequest::ConfigurationReloadRequest(
        dispatcher::domain::ConfigurationSnapshot snapshot
    )
        : snapshot_(std::move(snapshot))
    {
    }

    const dispatcher::domain::ConfigurationSnapshot&
        ConfigurationReloadRequest::snapshot() const noexcept
    {
        return snapshot_;
    }

    std::uint64_t ConfigurationReloadRequest::config_version() const noexcept
    {
        return snapshot_.config_version();
    }

    dispatcher::domain::ConfigurationStatus
        ConfigurationReloadRequest::status() const noexcept
    {
        return snapshot_.status();
    }
}