#pragma once

#include <dispatcher/api/telemetry_ingest_api_result.hpp>
#include <dispatcher/api/telemetry_ingest_request.hpp>

namespace dispatcher::api
{
    class TelemetryApi
    {
    public:
        virtual ~TelemetryApi() = default;

        [[nodiscard]] virtual TelemetryIngestApiResult ingest(
            const TelemetryIngestRequest& request
        ) = 0;
    };
}