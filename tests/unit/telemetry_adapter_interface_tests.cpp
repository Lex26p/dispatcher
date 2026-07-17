#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_adapter.hpp>
#include <dispatcher/telemetry/telemetry_adapter_capability.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_adapter_test_value(
        std::string tag_id,
        dispatcher::telemetry::TagValue value,
        std::uint64_t sequence = 1
    )
    {
        using dispatcher::domain::Quality;
        using dispatcher::domain::TagId;
        using dispatcher::telemetry::TelemetryValue;

        const auto now = TelemetryValue::Clock::now();

        return TelemetryValue(
            TagId{ std::move(tag_id) },
            std::move(value),
            Quality::Good,
            now,
            now,
            sequence
        );
    }

    class FakeTelemetryAdapter final
        : public dispatcher::telemetry::TelemetryAdapter
    {
    public:
        std::string name() const override
        {
            return "fake-adapter";
        }

        dispatcher::telemetry::TelemetryAdapterCapabilities capabilities()
            const noexcept override
        {
            return dispatcher::telemetry::TelemetryAdapterCapabilities{
                dispatcher::telemetry::TelemetryAdapterCapability::Connect,
                dispatcher::telemetry::TelemetryAdapterCapability::Disconnect,
                dispatcher::telemetry::TelemetryAdapterCapability::ReadCurrent,
                dispatcher::telemetry::TelemetryAdapterCapability::HealthCheck
            };
        }

        bool connected() const noexcept override
        {
            return connected_;
        }

        dispatcher::telemetry::TelemetryAdapterResult connect() override
        {
            connected_ = true;

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult disconnect() override
        {
            connected_ = false;

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult health_check() override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.health_check",
                    name(),
                    {},
                    "connected",
                    "adapter is not connected"
                );
            }

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult read_current(
            const dispatcher::domain::TagId& tag_id,
            dispatcher::telemetry::TelemetryValue& value
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.read_current",
                    name(),
                    tag_id.value(),
                    "connected",
                    "adapter is not connected"
                );
            }

            value = make_adapter_test_value(
                tag_id.value(),
                dispatcher::telemetry::TagValue(42.0),
                ++sequence_
            );

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult read_current_batch(
            const std::vector<dispatcher::domain::TagId>& tag_ids,
            std::vector<dispatcher::telemetry::TelemetryValue>& values
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.read_current_batch",
                    name(),
                    {},
                    "connected",
                    "adapter is not connected"
                );
            }

            values.clear();
            values.reserve(tag_ids.size());

            for (const auto& tag_id : tag_ids)
            {
                values.push_back(
                    make_adapter_test_value(
                        tag_id.value(),
                        dispatcher::telemetry::TagValue(42.0),
                        ++sequence_
                    )
                );
            }

            return dispatcher::telemetry::TelemetryAdapterResult::success();
        }

        dispatcher::telemetry::TelemetryAdapterResult write_current(
            const dispatcher::domain::TagId& tag_id,
            const dispatcher::telemetry::TelemetryValue& value
        ) override
        {
            if (!connected_)
            {
                return dispatcher::telemetry::TelemetryAdapterResult::failure(
                    dispatcher::telemetry::TelemetryAdapterStatus::NotConnected,
                    "adapter.write_current",
                    name(),
                    tag_id.value(),
                    "connected",
                    "adapter is not connected"
                );
            }

            last_written_tag_id_ = tag_id.value();
            last_written_value_sequence_ = value.sequence();

            return dispatcher::telemetry::TelemetryAdapterResult::failure(
                dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation,
                "adapter.write_current",
                name(),
                tag_id.value(),
                "capability",
                "write_current is not supported"
            );
        }

        [[nodiscard]] const std::string& last_written_tag_id()
            const noexcept
        {
            return last_written_tag_id_;
        }

        [[nodiscard]] std::uint64_t last_written_value_sequence()
            const noexcept
        {
            return last_written_value_sequence_;
        }

    private:
        bool connected_{ false };
        std::uint64_t sequence_{ 0 };
        std::string last_written_tag_id_;
        std::uint64_t last_written_value_sequence_{ 0 };
    };
}

TEST(TelemetryAdapterCapabilityTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::None
        ),
        "none"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::Connect
        ),
        "connect"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::Disconnect
        ),
        "disconnect"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::ReadCurrent
        ),
        "read_current"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::WriteCurrent
        ),
        "write_current"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::Poll
        ),
        "poll"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::Subscribe
        ),
        "subscribe"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::HealthCheck
        ),
        "health_check"
    );

    EXPECT_STREQ(
        dispatcher::telemetry::to_string(
            dispatcher::telemetry::TelemetryAdapterCapability::Browse
        ),
        "browse"
    );
}

TEST(TelemetryAdapterCapabilitiesTests, DefaultCapabilitiesAreEmpty)
{
    const dispatcher::telemetry::TelemetryAdapterCapabilities capabilities;

    EXPECT_TRUE(capabilities.empty());
    EXPECT_FALSE(capabilities.any());
    EXPECT_EQ(capabilities.mask(), 0u);

    EXPECT_FALSE(capabilities.can_connect());
    EXPECT_FALSE(capabilities.can_disconnect());
    EXPECT_FALSE(capabilities.can_read_current());
    EXPECT_FALSE(capabilities.can_write_current());
    EXPECT_FALSE(capabilities.can_poll());
    EXPECT_FALSE(capabilities.can_subscribe());
    EXPECT_FALSE(capabilities.can_health_check());
    EXPECT_FALSE(capabilities.can_browse());
}

TEST(TelemetryAdapterCapabilitiesTests, InitializerListAddsCapabilities)
{
    const dispatcher::telemetry::TelemetryAdapterCapabilities capabilities{
        dispatcher::telemetry::TelemetryAdapterCapability::Connect,
        dispatcher::telemetry::TelemetryAdapterCapability::Disconnect,
        dispatcher::telemetry::TelemetryAdapterCapability::ReadCurrent
    };

    EXPECT_FALSE(capabilities.empty());
    EXPECT_TRUE(capabilities.any());

    EXPECT_TRUE(capabilities.can_connect());
    EXPECT_TRUE(capabilities.can_disconnect());
    EXPECT_TRUE(capabilities.can_read_current());

    EXPECT_FALSE(capabilities.can_write_current());
    EXPECT_FALSE(capabilities.can_poll());
    EXPECT_FALSE(capabilities.can_subscribe());
    EXPECT_FALSE(capabilities.can_health_check());
    EXPECT_FALSE(capabilities.can_browse());
}

TEST(TelemetryAdapterCapabilitiesTests, AddAndRemoveCapabilities)
{
    dispatcher::telemetry::TelemetryAdapterCapabilities capabilities;

    capabilities.add(
        dispatcher::telemetry::TelemetryAdapterCapability::Connect
    );

    capabilities.add(
        dispatcher::telemetry::TelemetryAdapterCapability::HealthCheck
    );

    EXPECT_TRUE(capabilities.can_connect());
    EXPECT_TRUE(capabilities.can_health_check());

    capabilities.remove(
        dispatcher::telemetry::TelemetryAdapterCapability::Connect
    );

    EXPECT_FALSE(capabilities.can_connect());
    EXPECT_TRUE(capabilities.can_health_check());
}

TEST(TelemetryAdapterCapabilitiesTests, AllContainsEveryCapability)
{
    const auto capabilities =
        dispatcher::telemetry::TelemetryAdapterCapabilities::all();

    EXPECT_TRUE(capabilities.can_connect());
    EXPECT_TRUE(capabilities.can_disconnect());
    EXPECT_TRUE(capabilities.can_read_current());
    EXPECT_TRUE(capabilities.can_write_current());
    EXPECT_TRUE(capabilities.can_poll());
    EXPECT_TRUE(capabilities.can_subscribe());
    EXPECT_TRUE(capabilities.can_health_check());
    EXPECT_TRUE(capabilities.can_browse());
}

TEST(TelemetryAdapterInterfaceTests, FakeAdapterExposesIdentityAndCapabilities)
{
    const FakeTelemetryAdapter adapter;

    EXPECT_EQ(adapter.name(), "fake-adapter");
    EXPECT_FALSE(adapter.connected());

    const auto capabilities = adapter.capabilities();

    EXPECT_TRUE(capabilities.can_connect());
    EXPECT_TRUE(capabilities.can_disconnect());
    EXPECT_TRUE(capabilities.can_read_current());
    EXPECT_TRUE(capabilities.can_health_check());

    EXPECT_FALSE(capabilities.can_write_current());
    EXPECT_FALSE(capabilities.can_poll());
    EXPECT_FALSE(capabilities.can_subscribe());
    EXPECT_FALSE(capabilities.can_browse());
}

TEST(TelemetryAdapterInterfaceTests, ConnectAndDisconnectUpdateState)
{
    FakeTelemetryAdapter adapter;

    const auto connect_result = adapter.connect();

    EXPECT_TRUE(connect_result.ok());
    EXPECT_TRUE(adapter.connected());

    const auto health_result = adapter.health_check();

    EXPECT_TRUE(health_result.ok());

    const auto disconnect_result = adapter.disconnect();

    EXPECT_TRUE(disconnect_result.ok());
    EXPECT_FALSE(adapter.connected());
}

TEST(TelemetryAdapterInterfaceTests, HealthCheckFailsWhenDisconnected)
{
    FakeTelemetryAdapter adapter;

    const auto result = adapter.health_check();

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().operation(), "adapter.health_check");
    EXPECT_EQ(result.error().adapter_name(), "fake-adapter");
    EXPECT_EQ(result.error().field(), "connected");
    EXPECT_TRUE(result.retryable());
}

TEST(TelemetryAdapterInterfaceTests, ReadCurrentRequiresConnection)
{
    FakeTelemetryAdapter adapter;

    auto value = make_adapter_test_value(
        "tag-placeholder",
        dispatcher::telemetry::TagValue(0.0),
        0
    );

    const auto result = adapter.read_current(
        dispatcher::domain::TagId{ "tag-temperature" },
        value
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::NotConnected
    );
}

TEST(TelemetryAdapterInterfaceTests, ReadCurrentReturnsTelemetryValue)
{
    FakeTelemetryAdapter adapter;

    ASSERT_TRUE(adapter.connect().ok());

    auto value = make_adapter_test_value(
        "tag-placeholder",
        dispatcher::telemetry::TagValue(0.0),
        0
    );

    const auto result = adapter.read_current(
        dispatcher::domain::TagId{ "tag-temperature" },
        value
    );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(value.tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(value.sequence(), 1);
}

TEST(TelemetryAdapterInterfaceTests, ReadCurrentBatchReturnsTelemetryValues)
{
    FakeTelemetryAdapter adapter;

    ASSERT_TRUE(adapter.connect().ok());

    std::vector<dispatcher::domain::TagId> tag_ids{
        dispatcher::domain::TagId{"tag-temperature"},
        dispatcher::domain::TagId{"tag-pressure"}
    };

    std::vector<dispatcher::telemetry::TelemetryValue> values;

    const auto result = adapter.read_current_batch(
        tag_ids,
        values
    );

    EXPECT_TRUE(result.ok());

    ASSERT_EQ(values.size(), 2);

    EXPECT_EQ(values[0].tag_id(), dispatcher::domain::TagId{ "tag-temperature" });
    EXPECT_EQ(values[0].sequence(), 1);

    EXPECT_EQ(values[1].tag_id(), dispatcher::domain::TagId{ "tag-pressure" });
    EXPECT_EQ(values[1].sequence(), 2);
}

TEST(TelemetryAdapterInterfaceTests, WriteCurrentReturnsUnsupportedOperation)
{
    FakeTelemetryAdapter adapter;

    ASSERT_TRUE(adapter.connect().ok());

    const auto value = make_adapter_test_value(
        "tag-temperature",
        dispatcher::telemetry::TagValue(42.0),
        99
    );

    const auto result = adapter.write_current(
        dispatcher::domain::TagId{ "tag-temperature" },
        value
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation
    );

    EXPECT_EQ(adapter.last_written_tag_id(), "tag-temperature");
    EXPECT_EQ(adapter.last_written_value_sequence(), 99);
}