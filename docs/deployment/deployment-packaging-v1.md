# Dispatcher deployment packaging v1

## Purpose

This document describes the local deployment packaging flow for Dispatcher v1.8.

The deployment package is intended to provide a self-contained local package layout with backend binaries, frontend static assets, configuration templates, helper scripts, documentation, runtime data directories, and smoke validation scripts.

## Package version

Target release tag:

```text
v1.8.0-service-packaging
```

## Package root

Default generated package root:

```text
artifacts/package/dispatcher
```

Generated files under `artifacts/` are build outputs and must not be committed to Git.

## Package layout

Expected package structure:

```text
dispatcher/
  bin/
    dispatcher-http-server.exe
    dispatcher-e2e-smoke.exe
    runtime DLLs
  config/
    backend-runtime.json
    frontend-appsettings.json
  data/
  logs/
  scripts/
    Start-DispatcherBackend.ps1
    Test-DispatcherPackage.ps1
  wwwroot/
    index.html
    _framework/
    appsettings.json
  docs/
    http-api-contracts-v1.md
    blazor-local-development.md
    sqlite-storage-v1.md
    modbus-tcp-adapter-v1.md
    notification-delivery-v1.md
    auth-audit-v1.md
    release-v1.7.0-auth-audit.md
    deployment-packaging-v1.md
  deployment-package.json
```

## Repository-level scripts

The deployment flow uses these repository-level scripts:

```text
scripts/deployment/Test-DispatcherPackageLayout.ps1
scripts/deployment/Publish-DispatcherFrontend.ps1
scripts/deployment/Test-DispatcherFrontendPublish.ps1
scripts/deployment/Build-DispatcherPackage.ps1
scripts/deployment/Start-DispatcherBackend.ps1
scripts/deployment/Test-DispatcherPackage.ps1
```

## Package-local scripts

The generated package includes these scripts:

```text
scripts/Start-DispatcherBackend.ps1
scripts/Test-DispatcherPackage.ps1
```

The package-local scripts are copied from the repository-level deployment scripts.

## Configuration templates

Backend runtime template:

```text
deployment/templates/backend-runtime.json
```

Package destination:

```text
config/backend-runtime.json
```

Frontend appsettings template:

```text
deployment/templates/frontend-appsettings.json
```

Package destination:

```text
config/frontend-appsettings.json
```

The frontend publish script also copies the deployment frontend appsettings template into:

```text
wwwroot/appsettings.json
```

## Frontend publish flow

Publish frontend assets from the repository root:

```powershell
scripts\deployment\Publish-DispatcherFrontend.ps1 -Clean
```

Validate frontend publish output:

```powershell
scripts\deployment\Test-DispatcherFrontendPublish.ps1
```

Expected frontend publish output:

```text
artifacts/frontend/publish/wwwroot
```

## Full package build flow

Build the deployment package:

```powershell
scripts\deployment\Build-DispatcherPackage.ps1 -Clean
```

Build the deployment package and create a zip archive:

```powershell
scripts\deployment\Build-DispatcherPackage.ps1 -Clean -CreateZip
```

Expected zip output:

```text
artifacts/package/dispatcher-v1.8.0-service-packaging.zip
```

## Static package smoke validation

Run static package validation:

```powershell
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -SkipEndpointChecks -SkipE2ESmoke
```

This validates package structure, configuration JSON, package manifest, scripts, docs, and frontend assets.

## Full package smoke validation

Run full package smoke validation:

```powershell
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -StartBackend -Port 18081 -BackendReadyTimeoutSeconds 30
```

This validates:

```text
package structure
configuration files
deployment-package.json
frontend static assets
required documentation
package-local scripts
dispatcher-e2e-smoke.exe
packaged dispatcher-http-server.exe
/health
/ready
/api/v1/runtime
/api/v1/alarms
```

Port `18081` is recommended for local package validation when `18080` may already be used by a development server.

## Recommended local release-check flow

From the repository root:

```powershell
scripts\deployment\Test-DispatcherPackageLayout.ps1
scripts\deployment\Build-DispatcherPackage.ps1 -Clean
scripts\deployment\Test-DispatcherPackage.ps1 -PackageRoot artifacts\package\dispatcher -StartBackend -Port 18081 -BackendReadyTimeoutSeconds 30
scripts\deployment\Build-DispatcherPackage.ps1 -Clean -CreateZip
```

Then run the normal development verification:

```powershell
cmake --preset windows-vs-debug
cmake --build --preset windows-vs-debug
ctest --preset windows-vs-debug
dotnet build frontend\Dispatcher.Frontend.slnx
out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

## Runtime DLL handling

The package builder copies backend executables and required runtime DLLs into:

```text
artifacts/package/dispatcher/bin
```

This is required because packaged executables must be runnable outside the original build output directory.

A missing runtime DLL can cause Windows process exit code:

```text
-1073741515
```

This corresponds to a loader failure, commonly caused by a missing DLL.

## Git policy

The following files are source files and should be committed:

```text
deployment/package-layout.json
deployment/templates/backend-runtime.json
deployment/templates/frontend-appsettings.json
deployment/README.md
docs/deployment/deployment-packaging-v1.md
scripts/deployment/*.ps1
```

The following files are generated artifacts and should not be committed:

```text
artifacts/frontend/publish/
artifacts/package/dispatcher/
artifacts/package/dispatcher-v1.8.0-service-packaging.zip
```

The repository `.gitignore` should include:

```text
/artifacts/
```

## Current limitations

This deployment packaging version is a local packaging foundation.

It does not yet provide:

```text
Windows service installation
Windows service uninstall
signed release artifacts
installer generation
automatic production configuration loading by backend runtime
external reverse proxy configuration
TLS certificate management
```

These items can be added in later release stages.
