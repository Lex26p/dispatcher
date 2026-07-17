#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/api/api_status_mapping.hpp>
#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

TEST(ApiStatusMappingTests, MapsStorageStatuses)
{
    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::Success
        ),
        dispatcher::api::ApiStatus::Success
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::NotFound
        ),
        dispatcher::api::ApiStatus::NotFound
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::AlreadyExists
        ),
        dispatcher::api::ApiStatus::Conflict
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::Conflict
        ),
        dispatcher::api::ApiStatus::Conflict
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::ValidationError
        ),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::SerializationError
        ),
        dispatcher::api::ApiStatus::StorageError
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::IoError
        ),
        dispatcher::api::ApiStatus::StorageError
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::BackendUnavailable
        ),
        dispatcher::api::ApiStatus::StorageError
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::Timeout
        ),
        dispatcher::api::ApiStatus::Timeout
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::UnsupportedOperation
        ),
        dispatcher::api::ApiStatus::UnsupportedOperation
    );

    EXPECT_EQ(
        dispatcher::api::map_storage_status_to_api_status(
            dispatcher::storage::StorageStatus::UnknownError
        ),
        dispatcher::api::ApiStatus::InternalError
    );
}

TEST(ApiStatusMappingTests, MapsTelemetryIngestStatuses)
{
    EXPECT_EQ(
        dispatcher::api::map_telemetry_ingest_status_to_api_status(
            dispatcher::core::TelemetryIngestStatus::Accepted
        ),
        dispatcher::api::ApiStatus::Success
    );

    EXPECT_EQ(
        dispatcher::api::map_telemetry_ingest_status_to_api_status(
            dispatcher::core::TelemetryIngestStatus::AcceptedNoChange
        ),
        dispatcher::api::ApiStatus::Success
    );

    EXPECT_EQ(
        dispatcher::api::map_telemetry_ingest_status_to_api_status(
            dispatcher::core::TelemetryIngestStatus::UnknownTag
        ),
        dispatcher::api::ApiStatus::RuntimeRejected
    );

    EXPECT_EQ(
        dispatcher::api::map_telemetry_ingest_status_to_api_status(
            dispatcher::core::TelemetryIngestStatus::DataTypeMismatch
        ),
        dispatcher::api::ApiStatus::RuntimeRejected
    );

    EXPECT_EQ(
        dispatcher::api::telemetry_ingest_status_message(
            dispatcher::core::TelemetryIngestStatus::UnknownTag
        ),
        "unknown_tag"
    );
}

TEST(ApiStatusMappingTests, MapsAlarmAcknowledgementStatuses)
{
    EXPECT_EQ(
        dispatcher::api::map_alarm_acknowledgement_status_to_api_status(
            dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged
        ),
        dispatcher::api::ApiStatus::Success
    );

    EXPECT_EQ(
        dispatcher::api::map_alarm_acknowledgement_status_to_api_status(
            dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm
        ),
        dispatcher::api::ApiStatus::NotFound
    );

    EXPECT_EQ(
        dispatcher::api::map_alarm_acknowledgement_status_to_api_status(
            dispatcher::alarm::AlarmAcknowledgementStatus::NotActive
        ),
        dispatcher::api::ApiStatus::Conflict
    );

    EXPECT_EQ(
        dispatcher::api::map_alarm_acknowledgement_status_to_api_status(
            dispatcher::alarm::AlarmAcknowledgementStatus::AlreadyAcknowledged
        ),
        dispatcher::api::ApiStatus::Conflict
    );

    EXPECT_EQ(
        dispatcher::api::map_alarm_acknowledgement_status_to_api_status(
            dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand
        ),
        dispatcher::api::ApiStatus::ValidationError
    );

    EXPECT_STREQ(
        dispatcher::api::alarm_acknowledgement_status_message(
            dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm
        ),
        "unknown_alarm"
    );
}

TEST(ApiStatusMappingTests, MapsRuntimeProcessSummaryStatuses)
{
    dispatcher::runtime::DispatcherRuntimeProcessSummary accepted_summary;

    accepted_summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;

    EXPECT_EQ(
        dispatcher::api::map_runtime_process_summary_to_api_status(
            accepted_summary
        ),
        dispatcher::api::ApiStatus::Success
    );

    dispatcher::runtime::DispatcherRuntimeProcessSummary rejected_summary;

    rejected_summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::UnknownTag;

    EXPECT_EQ(
        dispatcher::api::map_runtime_process_summary_to_api_status(
            rejected_summary
        ),
        dispatcher::api::ApiStatus::RuntimeRejected
    );

    dispatcher::runtime::DispatcherRuntimeProcessSummary missing_condition_summary;

    missing_condition_summary.telemetry_status =
        dispatcher::core::TelemetryIngestStatus::Accepted;
    missing_condition_summary.missing_condition_count = 1;

    EXPECT_EQ(
        dispatcher::api::map_runtime_process_summary_to_api_status(
            missing_condition_summary
        ),
        dispatcher::api::ApiStatus::Conflict
    );
}