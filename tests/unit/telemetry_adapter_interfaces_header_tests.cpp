#include <dispatcher/telemetry/telemetry_adapter_interfaces.hpp>

#include <gtest/gtest.h>

TEST(TelemetryAdapterInterfacesHeaderTests, UmbrellaHeaderCanBeIncluded)
{
    const auto result =
        dispatcher::telemetry::TelemetryAdapterResult::success();

    EXPECT_TRUE(result.ok());

    const auto capabilities =
        dispatcher::telemetry::TelemetryAdapterCapabilities::none();

    EXPECT_TRUE(capabilities.empty());

    const dispatcher::telemetry::TelemetryReadRequest read_request;

    EXPECT_TRUE(read_request.empty());

    const dispatcher::telemetry::TelemetryWriteRequest write_request;

    EXPECT_TRUE(write_request.empty());

    const dispatcher::telemetry::TelemetryAdapterRegistry registry;

    EXPECT_TRUE(registry.empty());
}