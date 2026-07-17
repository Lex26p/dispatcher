#include <dispatcher/http/http_runtime_contract.hpp>

#include <dispatcher/http/http_json.hpp>

#include <vector>

namespace dispatcher::http
{
    HttpRuntimeSummary HttpRuntimeContract::default_summary()
    {
        return HttpRuntimeSummary{};
    }

    std::string HttpRuntimeContract::to_json(
        const HttpRuntimeSummary& summary
    )
    {
        const auto service_json =
            HttpJson::object(
                {
                    HttpJson::string_field(
                        "name",
                        summary.service_name
                    ),
                    HttpJson::string_field(
                        "component",
                        summary.service_component
                    )
                }
            );

        const auto runtime_json =
            HttpJson::object(
                {
                    HttpJson::string_field(
                        "state",
                        summary.runtime_state
                    ),
                    HttpJson::bool_field(
                        "started",
                        summary.started
                    ),
                    HttpJson::bool_field(
                        "configured",
                        summary.configured
                    ),
                    HttpJson::bool_field(
                        "accepting_requests",
                        summary.accepting_requests
                    )
                }
            );

        const auto telemetry_json =
            HttpJson::object(
                {
                    HttpJson::bool_field(
                        "configured",
                        summary.telemetry.configured
                    ),
                    HttpJson::string_field(
                        "source",
                        summary.telemetry.source
                    ),
                    HttpJson::uint_field(
                        "device_count",
                        summary.telemetry.device_count
                    ),
                    HttpJson::uint_field(
                        "tag_count",
                        summary.telemetry.tag_count
                    ),
                    HttpJson::null_field(
                        "last_batch_sequence"
                    ),
                    HttpJson::null_field(
                        "last_ingest_timestamp"
                    )
                }
            );

        const auto alarms_json =
            HttpJson::object(
                {
                    HttpJson::bool_field(
                        "available",
                        summary.alarms.available
                    ),
                    HttpJson::uint_field(
                        "active_count",
                        summary.alarms.active_count
                    ),
                    HttpJson::uint_field(
                        "unacknowledged_count",
                        summary.alarms.unacknowledged_count
                    ),
                    HttpJson::uint_field(
                        "shelved_count",
                        summary.alarms.shelved_count
                    ),
                    HttpJson::uint_field(
                        "suppressed_count",
                        summary.alarms.suppressed_count
                    )
                }
            );

        const auto api_json =
            HttpJson::object(
                {
                    HttpJson::string_field(
                        "endpoint",
                        summary.endpoint
                    ),
                    HttpJson::string_field(
                        "path",
                        summary.path
                    ),
                    HttpJson::string_field(
                        "method",
                        summary.method
                    ),
                    HttpJson::string_field(
                        "source",
                        summary.source
                    )
                }
            );

        return HttpJson::object(
            {
                HttpJson::uint_field(
                    "schema_version",
                    summary.schema_version
                ),

                    // Backward-compatible top-level fields.
                    HttpJson::string_field(
                        "status",
                        summary.status
                    ),
                    HttpJson::string_field(
                        "endpoint",
                        summary.endpoint
                    ),
                    HttpJson::string_field(
                        "path",
                        summary.path
                    ),
                    HttpJson::string_field(
                        "method",
                        summary.method
                    ),
                    HttpJson::string_field(
                        "source",
                        summary.source
                    ),

                    // Structured contract fields.
                    HttpJson::raw_field(
                        "service",
                        service_json
                    ),
                    HttpJson::raw_field(
                        "runtime",
                        runtime_json
                    ),
                    HttpJson::raw_field(
                        "telemetry",
                        telemetry_json
                    ),
                    HttpJson::raw_field(
                        "alarms",
                        alarms_json
                    ),
                    HttpJson::raw_field(
                        "api",
                        api_json
                    )
            }
        );
    }

    std::string HttpRuntimeContract::default_json()
    {
        return to_json(
            default_summary()
        );
    }
}