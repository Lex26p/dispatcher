#pragma once

#include <dispatcher/domain/configuration_snapshot.hpp>
#include <dispatcher/domain/configuration_status.hpp>

#include <cstdint>

namespace dispatcher::api
{
    class ConfigurationReloadRequest
    {
    public:
        explicit ConfigurationReloadRequest(
            dispatcher::domain::ConfigurationSnapshot snapshot
        );

        [[nodiscard]] const dispatcher::domain::ConfigurationSnapshot&
            snapshot() const noexcept;

        [[nodiscard]] std::uint64_t config_version() const noexcept;

        [[nodiscard]] dispatcher::domain::ConfigurationStatus status()
            const noexcept;

    private:
        dispatcher::domain::ConfigurationSnapshot snapshot_;
    };
}