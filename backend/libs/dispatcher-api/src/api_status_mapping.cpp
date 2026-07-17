#include <dispatcher/api/api_status_mapping.hpp>

namespace dispatcher::api
{
    ApiStatus map_storage_status_to_api_status(
        dispatcher::storage::StorageStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::storage::StorageStatus::Success:
            return ApiStatus::Success;

        case dispatcher::storage::StorageStatus::NotFound:
            return ApiStatus::NotFound;

        case dispatcher::storage::StorageStatus::AlreadyExists:
            return ApiStatus::Conflict;

        case dispatcher::storage::StorageStatus::Conflict:
            return ApiStatus::Conflict;

        case dispatcher::storage::StorageStatus::ValidationError:
            return ApiStatus::ValidationError;

        case dispatcher::storage::StorageStatus::SerializationError:
            return ApiStatus::StorageError;

        case dispatcher::storage::StorageStatus::IoError:
            return ApiStatus::StorageError;

        case dispatcher::storage::StorageStatus::BackendUnavailable:
            return ApiStatus::StorageError;

        case dispatcher::storage::StorageStatus::Timeout:
            return ApiStatus::Timeout;

        case dispatcher::storage::StorageStatus::UnsupportedOperation:
            return ApiStatus::UnsupportedOperation;

        case dispatcher::storage::StorageStatus::UnknownError:
            return ApiStatus::InternalError;
        }

        return ApiStatus::InternalError;
    }

    ApiStatus map_configuration_io_status_to_api_status(
        dispatcher::config::ConfigurationIoStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::config::ConfigurationIoStatus::Success:
            return ApiStatus::Success;

        case dispatcher::config::ConfigurationIoStatus::UnsupportedFormat:
            return ApiStatus::UnsupportedOperation;

        case dispatcher::config::ConfigurationIoStatus::ValidationError:
            return ApiStatus::ValidationError;

        case dispatcher::config::ConfigurationIoStatus::ParseError:
            return ApiStatus::BadRequest;

        case dispatcher::config::ConfigurationIoStatus::SerializationError:
            return ApiStatus::InternalError;

        case dispatcher::config::ConfigurationIoStatus::NotFound:
            return ApiStatus::NotFound;

        case dispatcher::config::ConfigurationIoStatus::Conflict:
            return ApiStatus::Conflict;

        case dispatcher::config::ConfigurationIoStatus::IoError:
            return ApiStatus::InternalError;

        case dispatcher::config::ConfigurationIoStatus::UnknownError:
            return ApiStatus::InternalError;
        }

        return ApiStatus::InternalError;
    }

    ApiStatus map_telemetry_ingest_status_to_api_status(
        dispatcher::core::TelemetryIngestStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::core::TelemetryIngestStatus::Accepted:
            return ApiStatus::Success;

        case dispatcher::core::TelemetryIngestStatus::AcceptedNoChange:
            return ApiStatus::Success;

        case dispatcher::core::TelemetryIngestStatus::UnknownTag:
            return ApiStatus::RuntimeRejected;

        case dispatcher::core::TelemetryIngestStatus::DisabledTag:
            return ApiStatus::RuntimeRejected;

        case dispatcher::core::TelemetryIngestStatus::DataTypeMismatch:
            return ApiStatus::RuntimeRejected;

        case dispatcher::core::TelemetryIngestStatus::StaleSequence:
            return ApiStatus::RuntimeRejected;

        case dispatcher::core::TelemetryIngestStatus::FutureSourceTimestamp:
            return ApiStatus::RuntimeRejected;

        case dispatcher::core::TelemetryIngestStatus::BadQuality:
            return ApiStatus::RuntimeRejected;
        }

        return ApiStatus::InternalError;
    }

    ApiStatus map_alarm_acknowledgement_status_to_api_status(
        dispatcher::alarm::AlarmAcknowledgementStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged:
            return ApiStatus::Success;

        case dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm:
            return ApiStatus::NotFound;

        case dispatcher::alarm::AlarmAcknowledgementStatus::NotActive:
            return ApiStatus::Conflict;

        case dispatcher::alarm::AlarmAcknowledgementStatus::AlreadyAcknowledged:
            return ApiStatus::Conflict;

        case dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand:
            return ApiStatus::ValidationError;
        }

        return ApiStatus::InternalError;
    }

    ApiStatus map_runtime_process_summary_to_api_status(
        const dispatcher::runtime::DispatcherRuntimeProcessSummary& summary
    ) noexcept
    {
        if (summary.successful())
        {
            return ApiStatus::Success;
        }

        if (summary.telemetry_rejected())
        {
            return map_telemetry_ingest_status_to_api_status(
                summary.telemetry_status
            );
        }

        if (summary.has_missing_conditions())
        {
            return ApiStatus::Conflict;
        }

        return ApiStatus::InternalError;
    }

    std::string telemetry_ingest_status_message(
        dispatcher::core::TelemetryIngestStatus status
    )
    {
        return std::string(dispatcher::core::to_string(status));
    }

    const char* alarm_acknowledgement_status_message(
        dispatcher::alarm::AlarmAcknowledgementStatus status
    ) noexcept
    {
        switch (status)
        {
        case dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged:
            return "acknowledged";

        case dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm:
            return "unknown_alarm";

        case dispatcher::alarm::AlarmAcknowledgementStatus::NotActive:
            return "not_active";

        case dispatcher::alarm::AlarmAcknowledgementStatus::AlreadyAcknowledged:
            return "already_acknowledged";

        case dispatcher::alarm::AlarmAcknowledgementStatus::InvalidCommand:
            return "invalid_command";
        }

        return "internal_error";
    }
}