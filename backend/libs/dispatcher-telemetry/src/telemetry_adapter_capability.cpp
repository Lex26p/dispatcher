#include <dispatcher/telemetry/telemetry_adapter_capability.hpp>

namespace dispatcher::telemetry
{
    const char* to_string(
        TelemetryAdapterCapability capability
    ) noexcept
    {
        switch (capability)
        {
        case TelemetryAdapterCapability::None:
            return "none";

        case TelemetryAdapterCapability::Connect:
            return "connect";

        case TelemetryAdapterCapability::Disconnect:
            return "disconnect";

        case TelemetryAdapterCapability::ReadCurrent:
            return "read_current";

        case TelemetryAdapterCapability::WriteCurrent:
            return "write_current";

        case TelemetryAdapterCapability::Poll:
            return "poll";

        case TelemetryAdapterCapability::Subscribe:
            return "subscribe";

        case TelemetryAdapterCapability::HealthCheck:
            return "health_check";

        case TelemetryAdapterCapability::Browse:
            return "browse";
        }

        return "unknown";
    }

    TelemetryAdapterCapabilities::TelemetryAdapterCapabilities(
        std::initializer_list<TelemetryAdapterCapability> capabilities
    ) noexcept
    {
        for (const auto capability : capabilities)
        {
            add(capability);
        }
    }
}