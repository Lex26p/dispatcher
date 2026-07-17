# Dispatcher HTTP API contracts v1

## Purpose

This document describes the current HTTP API contracts exposed by the Dispatcher C++ HTTP backend and consumed by the Blazor WebAssembly frontend.

The current HTTP backend is provided by:

```text
dispatcher-http-server
```

Local development URL:

```text
http://127.0.0.1:18080
```

Current frontend local development URL:

```text
http://localhost:5077
```

## Contract stability rules

The current API contracts are intentionally small but should remain compatible with the Blazor frontend.

Compatibility rules:

```text
Do not remove existing top-level fields without a planned frontend migration.
Do not rename existing fields without a planned frontend migration.
Prefer adding new fields over changing existing fields.
Use schema_version for structured responses.
Keep endpoint/path/method/source fields for diagnostics.
Keep CORS support enabled for local Blazor WebAssembly development.
```

## Common response behavior

Successful API responses use JSON.

Expected content type:

```text
application/json
```

CORS headers are included in normal responses, not-found responses and preflight responses.

Current CORS headers:

```text
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization, X-Correlation-Id, X-Request-Id
Access-Control-Max-Age: 600
```

Preflight requests:

```text
OPTIONS <path>
```

Expected preflight response:

```text
HTTP 204 No Content
```

## Endpoint overview

Current endpoints:

```text
GET /health
GET /ready
GET /api/v1/runtime
GET /api/v1/alarms
```

## GET /health

### Purpose

Returns backend health status.

### Current response shape

```json
{
  "status": "healthy",
  "ready": true,
  "check_count": 2,
  "healthy_count": 2,
  "degraded_count": 0,
  "unhealthy_count": 0,
  "invalid_count": 0
}
```

### Fields

| Field             |   Type | Description                             |
| ----------------- | -----: | --------------------------------------- |
| `status`          | string | Overall health status.                  |
| `ready`           |   bool | Whether the system is considered ready. |
| `check_count`     | number | Total health check count.               |
| `healthy_count`   | number | Number of healthy checks.               |
| `degraded_count`  | number | Number of degraded checks.              |
| `unhealthy_count` | number | Number of unhealthy checks.             |
| `invalid_count`   | number | Number of invalid checks.               |

### Current frontend DTO

```text
HealthResponseDto
```

### Current frontend pages

```text
Dashboard
Health
```

## GET /ready

### Purpose

Returns backend readiness status.

### Current response shape

```json
{
  "status": "ready",
  "ready": true,
  "readiness_blockers": false,
  "invalid_checks": false,
  "check_count": 2
}
```

### Fields

| Field                |   Type | Description                                                   |
| -------------------- | -----: | ------------------------------------------------------------- |
| `status`             | string | Readiness status.                                             |
| `ready`              |   bool | Whether backend is ready to serve operator/frontend requests. |
| `readiness_blockers` |   bool | Whether readiness blockers are present.                       |
| `invalid_checks`     |   bool | Whether invalid readiness checks exist.                       |
| `check_count`        | number | Total readiness check count.                                  |

### Current frontend DTO

```text
ReadinessResponseDto
```

### Current frontend pages

```text
Health
```

## GET /api/v1/runtime

### Purpose

Returns structured runtime summary data for the frontend.

This endpoint is used by the frontend Dashboard and Runtime pages.

### Compatibility note

The endpoint keeps backward-compatible top-level fields:

```text
status
endpoint
path
method
source
```

New structured fields are added under:

```text
service
runtime
telemetry
alarms
api
```

### Current response shape

```json
{
  "schema_version": 1,
  "status": "available",
  "endpoint": "runtime",
  "path": "/api/v1/runtime",
  "method": "GET",
  "source": "dispatcher-http",
  "service": {
    "name": "dispatcher",
    "component": "runtime"
  },
  "runtime": {
    "state": "running",
    "started": true,
    "configured": true,
    "accepting_requests": true
  },
  "telemetry": {
    "configured": true,
    "source": "in_memory",
    "device_count": 0,
    "tag_count": 0,
    "last_batch_sequence": null,
    "last_ingest_timestamp": null
  },
  "alarms": {
    "available": true,
    "active_count": 0,
    "unacknowledged_count": 0,
    "shelved_count": 0,
    "suppressed_count": 0
  },
  "api": {
    "endpoint": "runtime",
    "path": "/api/v1/runtime",
    "method": "GET",
    "source": "dispatcher-http"
  }
}
```

### Top-level fields

| Field            |   Type | Description                               |
| ---------------- | -----: | ----------------------------------------- |
| `schema_version` | number | Runtime contract schema version.          |
| `status`         | string | Overall runtime endpoint status.          |
| `endpoint`       | string | Logical endpoint name.                    |
| `path`           | string | HTTP API path.                            |
| `method`         | string | HTTP method.                              |
| `source`         | string | Backend component producing the response. |

### `service` object

| Field       |   Type | Description             |
| ----------- | -----: | ----------------------- |
| `name`      | string | Service name.           |
| `component` | string | Service component name. |

### `runtime` object

| Field                |   Type | Description                            |
| -------------------- | -----: | -------------------------------------- |
| `state`              | string | Runtime state.                         |
| `started`            |   bool | Whether runtime is started.            |
| `configured`         |   bool | Whether runtime has configuration.     |
| `accepting_requests` |   bool | Whether runtime is accepting requests. |

### `telemetry` object

| Field                   |           Type | Description                                    |
| ----------------------- | -------------: | ---------------------------------------------- |
| `configured`            |           bool | Whether telemetry is configured.               |
| `source`                |         string | Telemetry source.                              |
| `device_count`          |         number | Number of configured devices.                  |
| `tag_count`             |         number | Number of configured tags.                     |
| `last_batch_sequence`   | number or null | Last telemetry batch sequence, if available.   |
| `last_ingest_timestamp` | string or null | Last telemetry ingest timestamp, if available. |

### `alarms` object

| Field                  |   Type | Description                           |
| ---------------------- | -----: | ------------------------------------- |
| `available`            |   bool | Whether alarm subsystem is available. |
| `active_count`         | number | Active alarm count.                   |
| `unacknowledged_count` | number | Unacknowledged alarm count.           |
| `shelved_count`        | number | Shelved alarm count.                  |
| `suppressed_count`     | number | Suppressed alarm count.               |

### `api` object

| Field      |   Type | Description                               |
| ---------- | -----: | ----------------------------------------- |
| `endpoint` | string | Logical endpoint name.                    |
| `path`     | string | HTTP API path.                            |
| `method`   | string | HTTP method.                              |
| `source`   | string | Backend component producing the response. |

### Current frontend DTO

```text
RuntimeResponseDto
RuntimeServiceDto
RuntimeStateDto
RuntimeTelemetryDto
RuntimeAlarmSummaryDto
RuntimeApiDto
```

### Current frontend pages

```text
Dashboard
Runtime
```

## GET /api/v1/alarms

### Purpose

Returns structured alarm summary data and alarm item list for the frontend.

This endpoint is used by the frontend Dashboard and Alarms pages.

### Compatibility note

The endpoint keeps backward-compatible top-level fields:

```text
status
endpoint
path
method
source
items
```

New structured fields are added under:

```text
summary
severity
states
api
```

### Current response shape

```json
{
  "schema_version": 1,
  "status": "available",
  "endpoint": "alarms",
  "path": "/api/v1/alarms",
  "method": "GET",
  "source": "dispatcher-http",
  "summary": {
    "available": true,
    "total_count": 0,
    "active_count": 0,
    "unacknowledged_count": 0,
    "shelved_count": 0,
    "suppressed_count": 0,
    "inhibited_count": 0
  },
  "severity": {
    "critical_count": 0,
    "high_count": 0,
    "medium_count": 0,
    "low_count": 0,
    "info_count": 0
  },
  "states": {
    "active_count": 0,
    "acknowledged_count": 0,
    "unacknowledged_count": 0,
    "shelved_count": 0,
    "suppressed_count": 0,
    "inhibited_count": 0
  },
  "items": [],
  "api": {
    "endpoint": "alarms",
    "path": "/api/v1/alarms",
    "method": "GET",
    "source": "dispatcher-http"
  }
}
```

### Top-level fields

| Field            |   Type | Description                               |
| ---------------- | -----: | ----------------------------------------- |
| `schema_version` | number | Alarm contract schema version.            |
| `status`         | string | Overall alarm endpoint status.            |
| `endpoint`       | string | Logical endpoint name.                    |
| `path`           | string | HTTP API path.                            |
| `method`         | string | HTTP method.                              |
| `source`         | string | Backend component producing the response. |
| `items`          |  array | Alarm item array.                         |

### `summary` object

| Field                  |   Type | Description                           |
| ---------------------- | -----: | ------------------------------------- |
| `available`            |   bool | Whether alarm subsystem is available. |
| `total_count`          | number | Total alarm item count.               |
| `active_count`         | number | Active alarm count.                   |
| `unacknowledged_count` | number | Unacknowledged alarm count.           |
| `shelved_count`        | number | Shelved alarm count.                  |
| `suppressed_count`     | number | Suppressed alarm count.               |
| `inhibited_count`      | number | Inhibited alarm count.                |

### `severity` object

| Field            |   Type | Description                |
| ---------------- | -----: | -------------------------- |
| `critical_count` | number | Critical alarm count.      |
| `high_count`     | number | High alarm count.          |
| `medium_count`   | number | Medium alarm count.        |
| `low_count`      | number | Low alarm count.           |
| `info_count`     | number | Informational alarm count. |

### `states` object

| Field                  |   Type | Description                 |
| ---------------------- | -----: | --------------------------- |
| `active_count`         | number | Active alarm count.         |
| `acknowledged_count`   | number | Acknowledged alarm count.   |
| `unacknowledged_count` | number | Unacknowledged alarm count. |
| `shelved_count`        | number | Shelved alarm count.        |
| `suppressed_count`     | number | Suppressed alarm count.     |
| `inhibited_count`      | number | Inhibited alarm count.      |

### `items` array item

| Field          |   Type | Description                     |
| -------------- | -----: | ------------------------------- |
| `id`           | string | Alarm identifier.               |
| `name`         | string | Alarm name.                     |
| `tag`          | string | Associated tag identifier/name. |
| `severity`     | string | Alarm severity.                 |
| `state`        | string | Alarm state.                    |
| `message`      | string | Operator-facing alarm message.  |
| `source`       | string | Source component.               |
| `active`       |   bool | Whether alarm is active.        |
| `acknowledged` |   bool | Whether alarm is acknowledged.  |
| `shelved`      |   bool | Whether alarm is shelved.       |
| `suppressed`   |   bool | Whether alarm is suppressed.    |
| `inhibited`    |   bool | Whether alarm is inhibited.     |

### `api` object

| Field      |   Type | Description                               |
| ---------- | -----: | ----------------------------------------- |
| `endpoint` | string | Logical endpoint name.                    |
| `path`     | string | HTTP API path.                            |
| `method`   | string | HTTP method.                              |
| `source`   | string | Backend component producing the response. |

### Current frontend DTO

```text
AlarmsResponseDto
AlarmSummaryDto
AlarmSeveritySummaryDto
AlarmStateSummaryDto
AlarmItemDto
AlarmApiDto
```

### Current frontend pages

```text
Dashboard
Alarms
```

## Frontend API client

Frontend pages must use:

```text
DispatcherBackendApiClient
```

Pages must not create direct `HttpClient` calls.

Current call chain:

```text
Razor page
  -> DispatcherBackendApiClient
  -> DispatcherBackendHttpClient
  -> HttpClient
  -> dispatcher-http-server
```

## Error handling expectations

Frontend behavior when backend is unavailable:

```text
Show an error alert.
Keep page navigation working.
Allow manual reload.
Do not crash the Blazor application.
```

Current frontend helper:

```text
ApiCallResult<T>
```

## Manual verification commands

Build frontend:

```powershell
dotnet restore frontend\Dispatcher.Frontend.slnx
dotnet build frontend\Dispatcher.Frontend.slnx
```

Build backend and run tests:

```powershell
cmake --preset windows-vs-debug
cmake --build --preset windows-vs-debug
ctest --preset windows-vs-debug
```

Run backend:

```powershell
.\out\build\windows-vs-debug\backend\apps\dispatcher-http-server\Debug\dispatcher-http-server.exe 127.0.0.1 18080
```

Run frontend:

```powershell
dotnet run --project frontend\Dispatcher.Web\Dispatcher.Web.csproj
```

Check raw JSON:

```powershell
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:18080/health | Select-Object -ExpandProperty Content
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:18080/ready | Select-Object -ExpandProperty Content
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:18080/api/v1/runtime | Select-Object -ExpandProperty Content
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:18080/api/v1/alarms | Select-Object -ExpandProperty Content
```

Check CORS:

```powershell
$response = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:18080/health" -Headers @{ Origin = "http://localhost:5077" }

$response.Headers["Access-Control-Allow-Origin"]
$response.Headers["Access-Control-Allow-Methods"]
$response.Headers["Access-Control-Allow-Headers"]
```

Expected CORS result:

```text
*
GET, POST, DELETE, OPTIONS
Content-Type, Authorization, X-Correlation-Id, X-Request-Id
```

Check frontend pages:

```text
http://localhost:5077/
http://localhost:5077/health
http://localhost:5077/runtime
http://localhost:5077/alarms
http://localhost:5077/api-diagnostics
```

## Known limitations

Current limitations:

```text
Runtime contract currently exposes summary fields but still uses default in-memory values.
Alarm contract currently exposes structured summary fields but not real alarm engine state yet.
Alarm items array is currently empty in the default backend response.
No OpenAPI document is generated yet.
No generated C# client is used yet.
No frontend automated API contract tests exist yet.
```

These limitations are accepted for the current `v1.3.0-blazor-integration` stage.
