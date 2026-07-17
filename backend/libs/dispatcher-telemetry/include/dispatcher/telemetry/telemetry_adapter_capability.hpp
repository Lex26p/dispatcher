#pragma once

#include <cstdint>
#include <initializer_list>

namespace dispatcher::telemetry
{
    enum class TelemetryAdapterCapability : std::uint32_t
    {
        None = 0,

        Connect = 1u << 0,
        Disconnect = 1u << 1,

        ReadCurrent = 1u << 2,
        WriteCurrent = 1u << 3,

        Poll = 1u << 4,
        Subscribe = 1u << 5,

        HealthCheck = 1u << 6,
        Browse = 1u << 7
    };

    [[nodiscard]] const char* to_string(
        TelemetryAdapterCapability capability
    ) noexcept;

    class TelemetryAdapterCapabilities
    {
    public:
        using Mask = std::uint32_t;

        constexpr TelemetryAdapterCapabilities() noexcept = default;

        constexpr explicit TelemetryAdapterCapabilities(
            Mask mask
        ) noexcept
            : mask_(mask)
        {
        }

        TelemetryAdapterCapabilities(
            std::initializer_list<TelemetryAdapterCapability> capabilities
        ) noexcept;

        [[nodiscard]] static constexpr TelemetryAdapterCapabilities none()
            noexcept
        {
            return TelemetryAdapterCapabilities{};
        }

        [[nodiscard]] static constexpr TelemetryAdapterCapabilities all()
            noexcept
        {
            return TelemetryAdapterCapabilities{
                static_cast<Mask>(
                    static_cast<Mask>(TelemetryAdapterCapability::Connect)
                    | static_cast<Mask>(TelemetryAdapterCapability::Disconnect)
                    | static_cast<Mask>(TelemetryAdapterCapability::ReadCurrent)
                    | static_cast<Mask>(TelemetryAdapterCapability::WriteCurrent)
                    | static_cast<Mask>(TelemetryAdapterCapability::Poll)
                    | static_cast<Mask>(TelemetryAdapterCapability::Subscribe)
                    | static_cast<Mask>(TelemetryAdapterCapability::HealthCheck)
                    | static_cast<Mask>(TelemetryAdapterCapability::Browse)
                )
            };
        }

        [[nodiscard]] constexpr Mask mask() const noexcept
        {
            return mask_;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return mask_ == 0;
        }

        [[nodiscard]] constexpr bool any() const noexcept
        {
            return mask_ != 0;
        }

        [[nodiscard]] constexpr bool has(
            TelemetryAdapterCapability capability
        ) const noexcept
        {
            return (mask_ & static_cast<Mask>(capability)) != 0;
        }

        constexpr void add(TelemetryAdapterCapability capability) noexcept
        {
            mask_ |= static_cast<Mask>(capability);
        }

        constexpr void remove(TelemetryAdapterCapability capability) noexcept
        {
            mask_ &= ~static_cast<Mask>(capability);
        }

        [[nodiscard]] constexpr bool can_connect() const noexcept
        {
            return has(TelemetryAdapterCapability::Connect);
        }

        [[nodiscard]] constexpr bool can_disconnect() const noexcept
        {
            return has(TelemetryAdapterCapability::Disconnect);
        }

        [[nodiscard]] constexpr bool can_read_current() const noexcept
        {
            return has(TelemetryAdapterCapability::ReadCurrent);
        }

        [[nodiscard]] constexpr bool can_write_current() const noexcept
        {
            return has(TelemetryAdapterCapability::WriteCurrent);
        }

        [[nodiscard]] constexpr bool can_poll() const noexcept
        {
            return has(TelemetryAdapterCapability::Poll);
        }

        [[nodiscard]] constexpr bool can_subscribe() const noexcept
        {
            return has(TelemetryAdapterCapability::Subscribe);
        }

        [[nodiscard]] constexpr bool can_health_check() const noexcept
        {
            return has(TelemetryAdapterCapability::HealthCheck);
        }

        [[nodiscard]] constexpr bool can_browse() const noexcept
        {
            return has(TelemetryAdapterCapability::Browse);
        }

    private:
        Mask mask_{ 0 };
    };
}