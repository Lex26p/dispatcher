#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/api/api_status.hpp>
#include <dispatcher/core/telemetry_ingest_status.hpp>
#include <dispatcher/runtime/dispatcher_runtime_process_summary.hpp>
#include <dispatcher/storage/storage_status.hpp>
#include <dispatcher/config/configuration_io_status.hpp>

#include <string>

namespace dispatcher::api
{
    [[nodiscard]] ApiStatus map_storage_status_to_api_status(
        dispatcher::storage::StorageStatus status
    ) noexcept;

    [[nodiscard]] ApiStatus map_configuration_io_status_to_api_status(
        dispatcher::config::ConfigurationIoStatus status
    ) noexcept;

    [[nodiscard]] ApiStatus map_telemetry_ingest_status_to_api_status(
        dispatcher::core::TelemetryIngestStatus status
    ) noexcept;

    [[nodiscard]] ApiStatus map_alarm_acknowledgement_status_to_api_status(
        dispatcher::alarm::AlarmAcknowledgementStatus status
    ) noexcept;

    [[nodiscard]] ApiStatus map_runtime_process_summary_to_api_status(
        const dispatcher::runtime::DispatcherRuntimeProcessSummary& summary
    ) noexcept;

    [[nodiscard]] std::string telemetry_ingest_status_message(
        dispatcher::core::TelemetryIngestStatus status
    );

    [[nodiscard]] const char* alarm_acknowledgement_status_message(
        dispatcher::alarm::AlarmAcknowledgementStatus status
    ) noexcept;
}