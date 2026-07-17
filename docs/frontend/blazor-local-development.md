# Blazor WebAssembly local development

## Purpose

This document describes how to run and verify the Dispatcher Blazor WebAssembly frontend together with the C++ HTTP backend.

The frontend project is located at:

```text
frontend/Dispatcher.Web
```

The frontend solution is located at:

```text
frontend/Dispatcher.Frontend.slnx
```

The backend HTTP server executable is:

```text
out/build/windows-vs-debug/backend/apps/dispatcher-http-server/Debug/dispatcher-http-server.exe
```

## Local URLs

Backend HTTP server:

```text
http://127.0.0.1:18080
```

Blazor WebAssembly frontend:

```text
http://localhost:5077
```

The frontend reads backend configuration from:

```text
frontend/Dispatcher.Web/wwwroot/appsettings.json
frontend/Dispatcher.Web/wwwroot/appsettings.Development.json
```

Current backend endpoints:

```text
GET /health
GET /ready
GET /api/v1/runtime
GET /api/v1/alarms
```

## Build frontend

From repository root:

```powershell
cd C:\Projects\dispatcher

dotnet restore frontend\Dispatcher.Frontend.slnx
dotnet build frontend\Dispatcher.Frontend.slnx
```

Expected result:

```text
Build succeeded.
0 Warning(s)
0 Error(s)
```

## Build backend

From repository root:

```powershell
cd C:\Projects\dispatcher

cmake --preset windows-vs-debug
cmake --build --preset windows-vs-debug
ctest --preset windows-vs-debug
```

Expected result:

```text
100% tests passed, 0 tests failed out of 12
```

## Run backend HTTP server

Open PowerShell terminal 1:

```powershell
cd C:\Projects\dispatcher

.\out\build\windows-vs-debug\backend\apps\dispatcher-http-server\Debug\dispatcher-http-server.exe 127.0.0.1 18080
```

Expected output:

```text
dispatcher-http-server listening on http://127.0.0.1:18080
Available endpoints:
  GET /health
  GET /ready
  GET /api/v1/runtime
  GET /api/v1/alarms
Press Ctrl+C to stop.
```

## Run frontend

Open PowerShell terminal 2:

```powershell
cd C:\Projects\dispatcher

dotnet run --project frontend\Dispatcher.Web\Dispatcher.Web.csproj
```

Expected output contains a frontend URL, for example:

```text
Now listening on: http://localhost:5077
```

Open in browser:

```text
http://localhost:5077
```

## Verify backend endpoints directly

With the backend running:

```powershell
Invoke-RestMethod http://127.0.0.1:18080/health
Invoke-RestMethod http://127.0.0.1:18080/ready
Invoke-RestMethod http://127.0.0.1:18080/api/v1/runtime
Invoke-RestMethod http://127.0.0.1:18080/api/v1/alarms
```

Expected high-level results:

```text
/health             returns status healthy
/ready              returns status ready
/api/v1/runtime     returns status available
/api/v1/alarms      returns status available and items []
```

## Verify CORS headers

Use PowerShell 5.1 compatible commands with `-UseBasicParsing`.

Check normal GET response:

```powershell
$response = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:18080/health" -Headers @{ Origin = "http://localhost:5077" }

$response.Headers["Access-Control-Allow-Origin"]
$response.Headers["Access-Control-Allow-Methods"]
$response.Headers["Access-Control-Allow-Headers"]
```

Expected result:

```text
*
GET, POST, DELETE, OPTIONS
Content-Type, Authorization, X-Correlation-Id, X-Request-Id
```

Check preflight response:

```powershell
$preflight = Invoke-WebRequest -UseBasicParsing -Method OPTIONS -Uri "http://127.0.0.1:18080/health" -Headers @{ Origin = "http://localhost:5077"; "Access-Control-Request-Method" = "GET" }

$preflight.StatusCode
$preflight.Headers["Access-Control-Allow-Origin"]
$preflight.Headers["Access-Control-Allow-Methods"]
$preflight.Headers["Access-Control-Allow-Headers"]
```

Expected result:

```text
204
*
GET, POST, DELETE, OPTIONS
Content-Type, Authorization, X-Correlation-Id, X-Request-Id
```

## Verify frontend pages

With backend and frontend running, open:

```text
http://localhost:5077/
http://localhost:5077/health
http://localhost:5077/runtime
http://localhost:5077/alarms
http://localhost:5077/api-diagnostics
```

Expected results:

```text
Dashboard        shows backend health/runtime/alarm summary
Health           shows status healthy and readiness ready
Runtime          shows status available
Alarms           shows active items count 0
API Diagnostics  shows configured backend URLs
```

## Verify backend unavailable behavior

Stop backend with `Ctrl+C`.

Refresh frontend pages.

Expected result:

```text
Frontend shows error alert.
Frontend does not crash.
```

Start backend again and press Reload on frontend pages.

Expected result:

```text
Frontend loads backend data again.
```

## Full local verification checklist

Before committing frontend changes, run:

```powershell
cd C:\Projects\dispatcher

dotnet restore frontend\Dispatcher.Frontend.slnx
dotnet build frontend\Dispatcher.Frontend.slnx

cmake --preset windows-vs-debug
cmake --build --preset windows-vs-debug
ctest --preset windows-vs-debug

.\out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

Then manually verify:

```text
Backend starts on 127.0.0.1:18080.
Frontend starts on localhost:5077.
CORS headers are present.
Dashboard loads backend data.
Health page loads backend data.
Runtime page loads backend data.
Alarms page loads backend data.
API Diagnostics page opens.
```

## Git workflow

From repository root:

```powershell
cd C:\Projects\dispatcher

git status --untracked-files=all
git add .
git commit -m "Update Blazor frontend local development documentation"
git push
```

## Current known limitations

The frontend is functional as a foundation, but still intentionally limited.

Current limitations:

```text
Runtime page displays the current backend skeleton response.
Alarms page displays the current backend skeleton response.
No persistent frontend state.
No authentication UI.
No operator login flow.
No production routing/deployment profile.
No frontend automated test project yet.
```

These limitations are accepted for the current Blazor WebAssembly frontend foundation stage.
