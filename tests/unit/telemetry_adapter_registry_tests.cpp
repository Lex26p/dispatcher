#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/telemetry/tag_value.hpp>
#include <dispatcher/telemetry/telemetry_adapter.hpp>
#include <dispatcher/telemetry/telemetry_adapter_capability.hpp>
#include <dispatcher/telemetry/telemetry_adapter_registry.hpp>
#include <dispatcher/telemetry/telemetry_adapter_result.hpp>
#include <dispatcher/telemetry/telemetry_adapter_status.hpp>
#include <dispatcher/telemetry/telemetry_value.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    dispatcher::telemetry::TelemetryValue make_registry_test_value(
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

    class RegistryTestTelemetryAdapter final
        : public dispatcher::telemetry::TelemetryAdapter
    {
    public:
        explicit RegistryTestTelemetryAdapter(std::string name)
            : name_(std::move(name))
        {
        }

        std::string name() const override
        {
            return name_;
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
                    name_,
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
                    name_,
                    tag_id.value(),
                    "connected",
                    "adapter is not connected"
                );
            }

            value = make_registry_test_value(
                tag_id.value(),
                dispatcher::telemetry::TagValue(42.0),
                1
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
                    name_,
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
                    make_registry_test_value(
                        tag_id.value(),
                        dispatcher::telemetry::TagValue(42.0),
                        1
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
            (void)tag_id;
            (void)value;

            return dispatcher::telemetry::TelemetryAdapterResult::failure(
                dispatcher::telemetry::TelemetryAdapterStatus::UnsupportedOperation,
                "adapter.write_current",
                name_,
                {},
                "capability",
                "write_current is not supported"
            );
        }

    private:
        std::string name_;
        bool connected_{ false };
    };
}

TEST(TelemetryAdapterRegistryTests, DefaultRegistryIsEmpty)
{
    const dispatcher::telemetry::TelemetryAdapterRegistry registry;

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);

    EXPECT_FALSE(registry.contains("simulator"));
    EXPECT_EQ(registry.find("simulator"), nullptr);
    EXPECT_TRUE(registry.names().empty());
    EXPECT_TRUE(registry.adapters().empty());
}

TEST(TelemetryAdapterRegistryTests, RegisterAdapterStoresItByName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    const auto adapter =
        std::make_shared<RegistryTestTelemetryAdapter>("simulator");

    const auto result = registry.register_adapter(adapter);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(registry.empty());
    EXPECT_EQ(registry.size(), 1);

    EXPECT_TRUE(registry.contains("simulator"));

    const auto found = registry.find("simulator");

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name(), "simulator");
    EXPECT_EQ(found, adapter);
}

TEST(TelemetryAdapterRegistryTests, RegisterRejectsNullAdapter)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    const auto result = registry.register_adapter(nullptr);

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(
        result.error().operation(),
        "telemetry_adapter_registry.register"
    );

    EXPECT_EQ(result.error().field(), "adapter");
    EXPECT_EQ(registry.size(), 0);
}

TEST(TelemetryAdapterRegistryTests, RegisterRejectsEmptyAdapterName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    const auto result = registry.register_adapter(
        std::make_shared<RegistryTestTelemetryAdapter>("")
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidConfiguration
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().field(), "name");
    EXPECT_EQ(result.error().message(), "adapter name is empty");

    EXPECT_TRUE(registry.empty());
}

TEST(TelemetryAdapterRegistryTests, RegisterRejectsDuplicateAdapterName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("simulator")
        ).ok()
    );

    const auto result = registry.register_adapter(
        std::make_shared<RegistryTestTelemetryAdapter>("simulator")
    );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidConfiguration
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().resource(), "simulator");
    EXPECT_EQ(result.error().field(), "name");
    EXPECT_EQ(
        result.error().message(),
        "adapter name is already registered"
    );

    EXPECT_EQ(registry.size(), 1);
}

TEST(TelemetryAdapterRegistryTests, NamesAreReturnedSorted)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("zeta")
        ).ok()
    );

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("alpha")
        ).ok()
    );

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("middle")
        ).ok()
    );

    const auto names = registry.names();

    ASSERT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "middle");
    EXPECT_EQ(names[2], "zeta");
}

TEST(TelemetryAdapterRegistryTests, AdaptersAreReturnedSortedByName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("zeta")
        ).ok()
    );

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("alpha")
        ).ok()
    );

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("middle")
        ).ok()
    );

    const auto adapters = registry.adapters();

    ASSERT_EQ(adapters.size(), 3);
    EXPECT_EQ(adapters[0]->name(), "alpha");
    EXPECT_EQ(adapters[1]->name(), "middle");
    EXPECT_EQ(adapters[2]->name(), "zeta");
}

TEST(TelemetryAdapterRegistryTests, UnregisterRemovesAdapter)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("simulator")
        ).ok()
    );

    ASSERT_TRUE(registry.contains("simulator"));

    const auto result = registry.unregister_adapter("simulator");

    EXPECT_TRUE(result.ok());

    EXPECT_FALSE(registry.contains("simulator"));
    EXPECT_EQ(registry.find("simulator"), nullptr);
    EXPECT_TRUE(registry.empty());
}

TEST(TelemetryAdapterRegistryTests, UnregisterRejectsEmptyName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    const auto result = registry.unregister_adapter("");

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(
        result.error().operation(),
        "telemetry_adapter_registry.unregister"
    );

    EXPECT_EQ(result.error().field(), "name");
    EXPECT_EQ(result.error().message(), "adapter name is empty");
}

TEST(TelemetryAdapterRegistryTests, UnregisterRejectsUnknownName)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    const auto result = registry.unregister_adapter("missing");

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::telemetry::TelemetryAdapterStatus::InvalidRequest
    );

    ASSERT_TRUE(result.has_error());

    EXPECT_EQ(result.error().resource(), "missing");
    EXPECT_EQ(result.error().field(), "name");
    EXPECT_EQ(result.error().message(), "adapter is not registered");
}

TEST(TelemetryAdapterRegistryTests, ClearRemovesAllAdapters)
{
    dispatcher::telemetry::TelemetryAdapterRegistry registry;

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("simulator")
        ).ok()
    );

    ASSERT_TRUE(
        registry.register_adapter(
            std::make_shared<RegistryTestTelemetryAdapter>("opcua")
        ).ok()
    );

    ASSERT_EQ(registry.size(), 2);

    registry.clear();

    EXPECT_TRUE(registry.empty());
    EXPECT_EQ(registry.size(), 0);

    EXPECT_FALSE(registry.contains("simulator"));
    EXPECT_FALSE(registry.contains("opcua"));
}