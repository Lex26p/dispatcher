# Dispatcher deployment package layout

## Summary

This directory defines the deployment packaging contract for Dispatcher.

The package layout contract is described by:

```text
deployment/package-layout.json
```

The contract describes the expected structure of a packaged Dispatcher deployment, including backend binaries, frontend static assets, configuration files, scripts, runtime directories, documentation, and smoke checks.

## Intended package root

The intended package output root is:

```text
artifacts/package/dispatcher
```

Generated package outputs under `artifacts/` are build artifacts and should not be committed to Git.

## Expected package structure

The package root should contain:

```text
dispatcher/
  bin/
  config/
  data/
  logs/
  scripts/
  wwwroot/
  docs/
  deployment-package.json
```

## Directory purpose

`bin` contains backend executables and runtime DLL dependencies.

`config` contains deployment configuration templates and runtime configuration files.

`data` contains persistent local runtime data, including future SQLite database files.

`logs` contains runtime logs and file-based notification output.

`scripts` contains package-local run, validation, and service helper scripts.

`wwwroot` contains published Blazor WebAssembly static assets.

`docs` contains deployment-relevant documentation copied from the repository.

`deployment-package.json` describes the generated package.

## Backend binaries

The package contract requires:

```text
bin/dispatcher-http-server.exe
bin/dispatcher-e2e-smoke.exe
```

`dispatcher-http-server.exe` is the packaged HTTP API server.

`dispatcher-e2e-smoke.exe` is retained as a package-level smoke executable.

Runtime DLLs required by these executables are copied into `bin`.

## Frontend assets

The frontend package destination is:

```text
wwwroot
```

The frontend source project is:

```text
frontend/Dispatcher.Web/Dispatcher.Web.csproj
```

The frontend publish root is:

```text
artifacts/frontend/publish
```

The static assets source for the deployment package is:

```text
artifacts/frontend/publish/wwwroot
```

Publish the frontend from the repository root:

```powershell
scripts\deployment\Publish-DispatcherFrontend.ps1 -Clean
```

Validate the frontend publish output:

```powershell
scripts\deployment\Test-DispatcherFrontendPublish.ps1
```

The publish script copies the deployment frontend appsettings template into:

```text
artifacts/frontend/publish/wwwroot/appsettings.json
```

## Configuration

The package contract declares these configuration files:

```text
config/backend-runtime.json
config/frontend-appsettings.json
```

Source templates:

```text
deployment/templates/backend-runtime.json
deployment/templates/frontend-appsettings.json
```

## Scripts

Repository-level deployment scripts:

```text
scripts/deployment/Test-DispatcherPackageLayout.ps1
scripts/deployment/Publish-DispatcherFrontend.ps1
scripts/deployment/Test-DispatcherFrontendPublish.ps1
scripts/deployment/Build-DispatcherPackage.ps1
scripts/deployment/Start-DispatcherBackend.ps1
scripts/deployment/Test-DispatcherPackage.ps1
```

Package-local scripts declared by the layout contract:

```text
scripts/Start-DispatcherBackend.ps1
scripts/Test-DispatcherPackage.ps1
```

## Documentation

The package includes deployment-relevant documentation from:

```text
docs/api
docs/frontend
docs/storage
docs/telemetry
docs/notifications
docs/auth-audit
docs/deployment
docs/release
```

The deployment packaging guide is:

```text
docs/deployment/deployment-packaging-v1.md
```

Package destination:

```text
docs/deployment-packaging-v1.md
```

The exact documentation copy list is defined in `deployment/package-layout.json`.

## Layout validation

Validate the package layout contract from the repository root:

```powershell
scripts\deployment\Test-DispatcherPackageLayout.ps1
```

A successful validation means that the repository has the expected packaging contract file, required configuration templates, deployment helper scripts, source documentation, and frontend project path.

It does not mean that a full deployment package has already been built.

## Frontend publish flow

Publish frontend assets:

```powershell
scripts\deployment\Publish-DispatcherFrontend.ps1 -Clean
```

Validate frontend publish output:

```powershell
scripts\deployment\Test-DispatcherFrontendPublish.ps1
```

Expected output root:

```text
artifacts/frontend/publish/wwwroot
```

## Full package build flow

Build a full deployment package:

```powershell
scripts\deployment\Build-DispatcherPackage.ps1 -Clean
```

Build a full deployment package and create a zip archive:

```powershell
scripts\deployment\Build-DispatcherPackage.ps1 -Clean -CreateZip
```

Expected package root:

```text
artifacts/package/dispatcher
```

Optional archive location:

```text
artifacts/package/dispatcher-v1.8.0-service-packaging.zip
```

The package build script performs:

```text
backend configure
backend build
frontend publish
package directory creation
backend binary copy
runtime DLL copy
configuration template copy
package-local script copy
frontend wwwroot copy
documentation copy
deployment-package.json creation
optional zip archive creation
```

## Package smoke validation

Static package validation:

```powershell
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -SkipEndpointChecks -SkipE2ESmoke
```

Full package validation with package e2e smoke and endpoint checks:

```powershell
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -StartBackend -Port 18081 -BackendReadyTimeoutSeconds 30
```

This full validation starts the packaged backend from:

```text
artifacts/package/dispatcher/bin/dispatcher-http-server.exe
```

Then checks:

```text
bin/dispatcher-e2e-smoke.exe
/health
/ready
/api/v1/runtime
/api/v1/alarms
```

The validation script stops the backend process after endpoint checks.

## Current full release-check flow

Recommended full local deployment flow:

```powershell
scripts\deployment\Test-DispatcherPackageLayout.ps1
scripts\deployment\Build-DispatcherPackage.ps1 -Clean
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -StartBackend -Port 18081 -BackendReadyTimeoutSeconds 30
scripts\deployment\Build-DispatcherPackage.ps1 -Clean -CreateZip
```

## Known limitations

The following work is intentionally deferred to later steps or later releases:

```text
Windows service install/uninstall helpers
signed release artifacts
installer generation
```

