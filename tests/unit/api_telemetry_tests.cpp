#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/dispatcher_telemetry_api.hpp>
#include <dispatcher/api/telemetry_api.hpp>
#include <dispatcher/api/telemetry_ingest_api_result.hpp>
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
#include <dispatcher/telemetry/tag_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>

namespace
{
    dispatcher::domain::DeviceDefinition make_api_device()
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

    dispatcher::domain::TagDefinition make_api_tag()
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

    dispatcher::domain::ConfigurationSnapshot make_api_configuration()
    {
        dispatcher::domain::ConfigurationSnapshotBuilder builder;

        builder.config_version(7);
        builder.status(dispatcher::domain::ConfigurationStatus::Published);

        const auto device_result = builder.add_device(make_api_device());

        if (device_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add device to api configuration: "
                + device_result.errors().front().field
                + " - "
                + device_result.errors().front().message
            );
        }

        const auto tag_result = builder.add_tag(make_api_tag());

        if (tag_result.has_errors())
        {
            throw std::runtime_error(
                "failed to add tag to api configuration: "
                + tag_result.errors().front().field
                + " - "
                + tag_result.errors().front().message
            );
        }

        return builder.build();
    }

    dispatcher::api::TelemetryIngestRequest make_request(
        std::string tag_id = "tag-temperature",
        double value = 42.0,
        std::uint64_t sequence = 1
    )
    {
        return dispatcher::api::TelemetryIngestRequest{
            .tag_id = dispatcher::domain::TagId{std::move(tag_id)},
            .value = dispatcher::telemetry::TagValue(
                static_cast<double>(value)
            ),
            .quality = dispatcher::domain::Quality::Good,
            .sequence = sequence
        };
    }
}

TEST(TelemetryIngestRequestTests, RequestConvertsToTelemetryValue)
{
    const auto request = make_request(
        "tag-temperature",
        42.0,
        123
    );

    EXPECT_FALSE(request.has_source_timestamp());
    EXPECT_FALSE(request.has_ingest_timestamp());

    const auto telemetry_value = request.to_telemetry_value();

    EXPECT_EQ(
        telemetry_value.tag_id(),
        dispatcher::domain::TagId{ "tag-temperature" }
    );

    EXPECT_EQ(
        telemetry_value.value().as<double>(),
        42.0
    );

    EXPECT_EQ(
        telemetry_value.quality(),
        dispatcher::domain::Quality::Good
    );

    EXPECT_EQ(telemetry_value.sequence(), 123);
}

TEST(TelemetryIngestApiResultTests, SuccessResultContainsSummary)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary summary;

    summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    const auto result =
        dispatcher::api::TelemetryIngestApiResult::success(summary);

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.has_summary());

    EXPECT_EQ(
        result.summary().telemetry_status,
        dispatcher::core::TelemetryIngestStatus::Accepted
    );
}

TEST(TelemetryIngestApiResultTests, FailureResultDoesNotContainSummary)
{
    const auto result =
        dispatcher::api::TelemetryIngestApiResult::failure(
            dispatcher::api::ApiStatus::RuntimeRejected,
            "telemetry.ingest",
            "unknown-tag",
            {},
            "unknown_tag"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::RuntimeRejected
    );

    EXPECT_FALSE(result.has_summary());

    EXPECT_EQ(result.error().operation, "telemetry.ingest");
    EXPECT_EQ(result.error().resource, "unknown-tag");
    EXPECT_EQ(result.error().message, "unknown_tag");

    EXPECT_THROW(
        (void)result.summary(),
        std::logic_error
    );
}

TEST(DispatcherTelemetryApiTests, IngestAcceptedTelemetryValue)
{
    dispatcher::runtime::DispatcherRuntime runtime(make_api_configuration());
    dispatcher::api::DispatcherTelemetryApi api(runtime);

    dispatcher::api::TelemetryApi& telemetry_api = api;

    const auto result = telemetry_api.ingest(
        make_request(
            "tag-temperature",
            42.0,
            1
        )
    );

    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.has_summary());

    EXPECT_EQ(result.status(), dispatcher::api::ApiStatus::Success);
    EXPECT_TRUE(result.summary().telemetry_accepted());
    EXPECT_TRUE(result.summary().telemetry_stored());
    EXPECT_TRUE(result.summary().history_written());

    const auto runtime_snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(runtime_snapshot.telemetry.current_state_size, 1);
    EXPECT_EQ(runtime_snapshot.history.store_size, 1);
}

TEST(DispatcherTelemetryApiTests, IngestRejectedTelemetryValue)
{
    dispatcher::runtime::DispatcherRuntime runtime(make_api_configuration());
    dispatcher::api::DispatcherTelemetryApi api(runtime);

    dispatcher::api::TelemetryApi& telemetry_api = api;

    const auto result = telemetry_api.ingest(
        make_request(
            "unknown-tag",
            42.0,
            1
        )
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::api::ApiStatus::RuntimeRejected
    );

    EXPECT_FALSE(result.has_summary());

    EXPECT_EQ(result.error().operation, "telemetry.ingest");
    EXPECT_EQ(result.error().resource, "unknown-tag");
    EXPECT_EQ(result.error().message, "unknown_tag");

    const auto runtime_snapshot = runtime.runtime_snapshot();

    EXPECT_EQ(runtime_snapshot.telemetry.current_state_size, 0);
    EXPECT_EQ(runtime_snapshot.history.store_size, 0);
}

TEST(DispatcherTelemetryApiTests, ExposesWrappedRuntime)
{
    dispatcher::runtime::DispatcherRuntime runtime(make_api_configuration());
    dispatcher::api::DispatcherTelemetryApi api(runtime);

    EXPECT_EQ(&api.runtime(), &runtime);

    const auto& const_api = api;

    EXPECT_EQ(&const_api.runtime(), &runtime);
}