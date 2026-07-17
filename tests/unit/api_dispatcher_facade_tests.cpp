#include <dispatcher/api/api_page_request.hpp>
#include <dispatcher/api/configuration_query_request.hpp>
#include <dispatcher/api/dispatcher_api.hpp>
#include <dispatcher/api/history_query_request.hpp>
#include <dispatcher/api/telemetry_ingest_request.hpp>
#include <dispatcher/domain/configuration_snapshot_builder.hpp>
#include <dispatcher/domain/configuration_status.hpp>
#include <dispatcher/domain/data_type.hpp>
#include <dispatcher/domain/device_definition_builder.hpp>
#include <dispatcher/domain/history_policy.hpp>
#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/quality.hpp>
#include <dispatcher/domain/tag_definition_builder.hpp>
#include <dispatcher/runtime/dispatcher_runtime.hpp>
#include <dispatcher/storage/in_memory_storage_repository.hpp>
#include <dispatcher/telemetry/tag_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace
{
    dispatcher::domain::DeviceDefinition make_facade_device()
    {
        return dispatcher::domain::DeviceDefinitionBuilder{}
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .local_name("device-1")
            .display_name("Device 1")
            .enabled(true)
            .build();
    }

    dispatcher::domain::TagDefinition make_facade_tag()
    {
        return dispatcher::domain::TagDefinitionBuilder{}
            .tag_id(dispatcher::domain::TagId{ "tag-temperature" })
            .organization_id(dispatcher::domain::OrganizationId{ "org-1" })
            .site_id(dispatcher::domain::SiteId{ "site-1" })
            .area_id(dispatcher::domain::AreaId{ "area-1" })
            .device_id(dispatcher::domain::DeviceId{ "device-1" })
            .local_name("temperature")
            .display_name("Temperature")
            .data_type(dispatcher::domain::DataType::Float64)
            .history_policy(dispatcher::domain::HistoryPolicy::EveryPoll)
            .enabled(true)
            .build();
    }

    dispatcher::domain::ConfigurationSnapshot make_facade_configuration(
        std::uint64_t config_version = 7
    )
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(config_version);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(make_facade_device());

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device to facade configuration: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto tag_result = builder.add_tag(make_facade_tag());

        if (tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add tag to facade configuration: "
                + tag_result.errors().front().field
                + " - "
                + tag_result.errors().front().message
            );
        }

        return builder.build();
    }

    dispatcher::api::TelemetryIngestRequest make_facade_ingest_request(
        std::uint64_t sequence = 1
    )
    {
        return dispatcher::api::TelemetryIngestRequest{
            .tag_id = dispatcher::domain::TagId{"tag-temperature"},
            .value = dispatcher::telemetry::TagValue(42.0),
            .quality = dispatcher::domain::Quality::Good,
            .sequence = sequence
        };
    }
}

TEST(DispatcherApiFacadeTests, ExposesWrappedRuntime)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}

TEST(DispatcherApiFacadeTests, ExposesConcreteAndInterfaceApis)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    EXPECT_EQ(
        &api.dispatcher_runtime_api(),
        &static_cast<dispatcher::api::DispatcherRuntimeApi&>(
            api.runtime_api()
            )
    );

    EXPECT_EQ(
        &api.dispatcher_telemetry_api(),
        &static_cast<dispatcher::api::DispatcherTelemetryApi&>(
            api.telemetry_api()
            )
    );

    EXPECT_EQ(
        &api.dispatcher_alarm_api(),
        &static_cast<dispatcher::api::DispatcherAlarmApi&>(
            api.alarm_api()
            )
    );

    EXPECT_EQ(
        &api.dispatcher_history_api(),
        &static_cast<dispatcher::api::DispatcherHistoryApi&>(
            api.history_api()
            )
    );

    EXPECT_EQ(
        &api.dispatcher_configuration_api(),
        &static_cast<dispatcher::api::DispatcherConfigurationApi&>(
            api.configuration_api()
            )
    );
}

TEST(DispatcherApiFacadeTests, RuntimeApiWorksThroughFacade)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    const auto result = api.runtime_api().runtime_snapshot();

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_snapshot());

    EXPECT_EQ(result.snapshot().telemetry.current_state_size, 0);
    EXPECT_EQ(result.snapshot().history.store_size, 0);
    EXPECT_EQ(result.snapshot().alarm.event_store_size, 0);
}

TEST(DispatcherApiFacadeTests, TelemetryApiWorksThroughFacade)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    const auto result = api.telemetry_api().ingest(
        make_facade_ingest_request(1)
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_summary());

    EXPECT_TRUE(result.summary().telemetry_accepted());
    EXPECT_TRUE(result.summary().telemetry_stored());
    EXPECT_TRUE(result.summary().history_written());

    const auto snapshot_result = api.runtime_api().runtime_snapshot();

    ASSERT_TRUE(snapshot_result.ok());
    EXPECT_EQ(snapshot_result.snapshot().telemetry.current_state_size, 1);
    EXPECT_EQ(snapshot_result.snapshot().history.store_size, 1);
}

TEST(DispatcherApiFacadeTests, AlarmApiWorksThroughFacade)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    const auto operator_snapshot_result =
        api.alarm_api().operator_snapshot();

    ASSERT_TRUE(operator_snapshot_result.ok());
    ASSERT_TRUE(operator_snapshot_result.has_snapshot());

    EXPECT_EQ(
        operator_snapshot_result.snapshot().configured_alarm_count,
        0
    );

    const auto unacknowledged_result =
        api.alarm_api().unacknowledged_alarms();

    ASSERT_TRUE(unacknowledged_result.ok());
    ASSERT_TRUE(unacknowledged_result.has_alarms());

    EXPECT_TRUE(unacknowledged_result.empty());
}

TEST(DispatcherApiFacadeTests, HistoryApiWorksThroughFacade)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    const auto result = api.history_api().query(
        dispatcher::api::HistoryQueryRequest{
            .page = dispatcher::api::ApiPageRequest{
                .offset = 0,
                .limit = 25
            }
        }
    );

    ASSERT_TRUE(result.ok());

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);
    EXPECT_EQ(result.page().limit(), 25);
}

TEST(DispatcherApiFacadeTests, ConfigurationApiWorksThroughFacade)
{
    dispatcher::storage::InMemoryStorageRepository storage_repository;

    dispatcher::runtime::DispatcherRuntime runtime(
        make_facade_configuration(7),
        storage_repository
    );

    dispatcher::api::DispatcherApi api(runtime);

    const auto result = api.configuration_api().query(
        dispatcher::api::ConfigurationQueryRequest{
            .config_version = 7,
            .page = dispatcher::api::ApiPageRequest{
                .offset = 0,
                .limit = 25
            }
        }
    );

    ASSERT_TRUE(result.ok());

    ASSERT_EQ(result.snapshot_count(), 1);
    EXPECT_EQ(result.snapshots().front().config_version(), 7);
    EXPECT_EQ(result.page().returned_count(), 1);
}