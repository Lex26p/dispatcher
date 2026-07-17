[CmdletBinding()]
param(
    [string]$PackageRoot = "",
    [string]$HostAddress = "127.0.0.1",
    [int]$Port = 18080,
    [int]$StartupDelaySeconds = 1,
    [int]$BackendReadyTimeoutSeconds = 15,
    [int]$E2ESampleCount = 10000,
    [switch]$StartBackend,
    [switch]$SkipEndpointChecks,
    [switch]$SkipE2ESmoke,
    [switch]$SkipFrontendChecks
)

$ErrorActionPreference = "Stop"

function Resolve-PackageRoot {
    param(
        [string]$ExplicitPackageRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitPackageRoot)) {
        return (Resolve-Path $ExplicitPackageRoot).Path
    }

    $scriptDirectory = Split-Path -Parent $PSCommandPath
    $candidatePackageRoot = Split-Path -Parent $scriptDirectory

    if (Test-Path (Join-Path $candidatePackageRoot "bin")) {
        return (Resolve-Path $candidatePackageRoot).Path
    }

    $repositoryRoot = Split-Path -Parent (Split-Path -Parent $scriptDirectory)
    $defaultPackageRoot = Join-Path $repositoryRoot "artifacts\package\dispatcher"

    return (Resolve-Path $defaultPackageRoot).Path
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path $Path)) {
        throw "Missing $Description`: $Path"
    }

    Write-Host "[OK] $Description"
}

function Assert-FileNotEmpty {
    param(
        [string]$Path,
        [string]$Description
    )

    Assert-PathExists -Path $Path -Description $Description

    $item = Get-Item $Path

    if ($item.Length -le 0) {
        throw "$Description is empty: $Path"
    }

    Write-Host "[OK] $Description is not empty"
}

function Assert-StringNotEmpty {
    param(
        [string]$Value,
        [string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        throw "Missing or empty $Description"
    }

    Write-Host "[OK] $Description"
}

function Read-JsonFile {
    param(
        [string]$Path,
        [string]$Description
    )

    try {
        return Get-Content -Path $Path -Raw | ConvertFrom-Json
    }
    catch {
        throw "Invalid JSON in $Description`: $Path. $($_.Exception.Message)"
    }
}

function Assert-JsonSchemaVersion {
    param(
        [object]$Json,
        [int]$ExpectedSchemaVersion,
        [string]$Description
    )

    if ($Json.schemaVersion -ne $ExpectedSchemaVersion) {
        throw "Unsupported schemaVersion in $Description`: $($Json.schemaVersion)"
    }

    Write-Host "[OK] $Description schemaVersion"
}

function Assert-PowerShellSyntax {
    param(
        [string]$Path,
        [string]$Description
    )

    $content = Get-Content -Path $Path -Raw
    $parseErrors = $null

    [System.Management.Automation.PSParser]::Tokenize(
        $content,
        [ref]$parseErrors
    ) | Out-Null

    if ($null -ne $parseErrors -and @($parseErrors).Count -gt 0) {
        $messages = @($parseErrors) | ForEach-Object {
            "$($_.Message) at line $($_.Token.StartLine), column $($_.Token.StartColumn)"
        }

        throw "PowerShell syntax errors in $Description`: $Path`n$($messages -join "`n")"
    }

    Write-Host "[OK] $Description syntax"
}

function Invoke-EndpointCheck {
    param(
        [string]$Uri,
        [string]$Description
    )

    Write-Host "Checking $Description`: $Uri"

    $response = Invoke-RestMethod -Uri $Uri -UseBasicParsing

    if ($null -eq $response) {
        throw "Endpoint returned empty response: $Uri"
    }

    Write-Host "[OK] $Description"
}

function Wait-ForBackendReady {
    param(
        [string]$BaseUrl,
        [int]$TimeoutSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastError = $null

    while ((Get-Date) -lt $deadline) {
        try {
            $response = Invoke-RestMethod -Uri "$BaseUrl/health" -UseBasicParsing

            if ($null -ne $response) {
                Write-Host "[OK] backend is reachable"
                return
            }
        }
        catch {
            $lastError = $_.Exception.Message
        }

        Start-Sleep -Milliseconds 500
    }

    throw "Backend did not become reachable within $TimeoutSeconds seconds. Last error: $lastError"
}

function Validate-BackendConfiguration {
    param(
        [string]$Path
    )

    $json = Read-JsonFile -Path $Path -Description "backend runtime configuration"

    Assert-JsonSchemaVersion -Json $json -ExpectedSchemaVersion 1 -Description "backend runtime configuration"

    Assert-StringNotEmpty -Value $json.application.name -Description "backend configuration application.name"
    Assert-StringNotEmpty -Value $json.application.component -Description "backend configuration application.component"
    Assert-StringNotEmpty -Value $json.server.host -Description "backend configuration server.host"
    Assert-StringNotEmpty -Value $json.server.baseUrl -Description "backend configuration server.baseUrl"
    Assert-StringNotEmpty -Value $json.storage.provider -Description "backend configuration storage.provider"

    if ($json.server.port -le 0) {
        throw "backend configuration server.port must be greater than zero"
    }

    Write-Host "[OK] backend configuration server.port"
}

function Validate-FrontendConfiguration {
    param(
        [string]$Path
    )

    $json = Read-JsonFile -Path $Path -Description "frontend appsettings configuration"

    Assert-JsonSchemaVersion -Json $json -ExpectedSchemaVersion 1 -Description "frontend appsettings configuration"

    Assert-StringNotEmpty -Value $json.application.name -Description "frontend configuration application.name"
    Assert-StringNotEmpty -Value $json.application.component -Description "frontend configuration application.component"
    Assert-StringNotEmpty -Value $json.DispatcherApi.BaseUrl -Description "frontend configuration DispatcherApi.BaseUrl"
    Assert-StringNotEmpty -Value $json.DispatcherApi.HealthEndpoint -Description "frontend configuration DispatcherApi.HealthEndpoint"
    Assert-StringNotEmpty -Value $json.DispatcherApi.ReadinessEndpoint -Description "frontend configuration DispatcherApi.ReadinessEndpoint"
    Assert-StringNotEmpty -Value $json.DispatcherApi.RuntimeEndpoint -Description "frontend configuration DispatcherApi.RuntimeEndpoint"
    Assert-StringNotEmpty -Value $json.DispatcherApi.AlarmsEndpoint -Description "frontend configuration DispatcherApi.AlarmsEndpoint"

    if ($json.DispatcherApi.TimeoutSeconds -le 0) {
        throw "frontend configuration DispatcherApi.TimeoutSeconds must be greater than zero"
    }

    Write-Host "[OK] frontend configuration DispatcherApi.TimeoutSeconds"
}

function Validate-PackageManifest {
    param(
        [string]$Path
    )

    $json = Read-JsonFile -Path $Path -Description "deployment package manifest"

    Assert-JsonSchemaVersion -Json $json -ExpectedSchemaVersion 1 -Description "deployment package manifest"

    Assert-StringNotEmpty -Value $json.packageName -Description "deployment package manifest packageName"
    Assert-StringNotEmpty -Value $json.versionTag -Description "deployment package manifest versionTag"
    Assert-StringNotEmpty -Value $json.packageRoot -Description "deployment package manifest packageRoot"
    Assert-StringNotEmpty -Value $json.generatedAtUtc -Description "deployment package manifest generatedAtUtc"

    if ($json.fileCount -le 0) {
        throw "deployment package manifest fileCount must be greater than zero"
    }

    Write-Host "[OK] deployment package manifest fileCount"

    if ($json.directoryCount -le 0) {
        throw "deployment package manifest directoryCount must be greater than zero"
    }

    Write-Host "[OK] deployment package manifest directoryCount"
}

if ($Port -le 0) {
    throw "Port must be greater than zero."
}

if ($StartupDelaySeconds -lt 0) {
    throw "StartupDelaySeconds must not be negative."
}

if ($BackendReadyTimeoutSeconds -le 0) {
    throw "BackendReadyTimeoutSeconds must be greater than zero."
}

if ($E2ESampleCount -le 0) {
    throw "E2ESampleCount must be greater than zero."
}

$resolvedPackageRoot = Resolve-PackageRoot -ExplicitPackageRoot $PackageRoot

Write-Host "Dispatcher package smoke validation"
Write-Host "Package root: $resolvedPackageRoot"
Write-Host "Host address: $HostAddress"
Write-Host "Port: $Port"

Assert-PathExists -Path $resolvedPackageRoot -Description "package root"

$binDirectory = Join-Path $resolvedPackageRoot "bin"
$configDirectory = Join-Path $resolvedPackageRoot "config"
$dataDirectory = Join-Path $resolvedPackageRoot "data"
$logsDirectory = Join-Path $resolvedPackageRoot "logs"
$scriptsDirectory = Join-Path $resolvedPackageRoot "scripts"
$wwwrootDirectory = Join-Path $resolvedPackageRoot "wwwroot"
$docsDirectory = Join-Path $resolvedPackageRoot "docs"

Assert-PathExists -Path $binDirectory -Description "bin directory"
Assert-PathExists -Path $configDirectory -Description "config directory"
Assert-PathExists -Path $dataDirectory -Description "data directory"
Assert-PathExists -Path $logsDirectory -Description "logs directory"
Assert-PathExists -Path $scriptsDirectory -Description "scripts directory"
Assert-PathExists -Path $wwwrootDirectory -Description "wwwroot directory"
Assert-PathExists -Path $docsDirectory -Description "docs directory"

$backendExecutable = Join-Path $binDirectory "dispatcher-http-server.exe"
$e2eSmokeExecutable = Join-Path $binDirectory "dispatcher-e2e-smoke.exe"
$backendConfig = Join-Path $configDirectory "backend-runtime.json"
$frontendConfig = Join-Path $configDirectory "frontend-appsettings.json"
$startBackendScript = Join-Path $scriptsDirectory "Start-DispatcherBackend.ps1"
$packageSmokeScript = Join-Path $scriptsDirectory "Test-DispatcherPackage.ps1"
$packageManifest = Join-Path $resolvedPackageRoot "deployment-package.json"

Assert-FileNotEmpty -Path $backendExecutable -Description "backend executable"
Assert-FileNotEmpty -Path $e2eSmokeExecutable -Description "e2e smoke executable"
Assert-FileNotEmpty -Path $backendConfig -Description "backend runtime configuration"
Assert-FileNotEmpty -Path $frontendConfig -Description "frontend appsettings configuration"
Assert-FileNotEmpty -Path $startBackendScript -Description "start backend script"
Assert-FileNotEmpty -Path $packageSmokeScript -Description "package smoke script"
Assert-FileNotEmpty -Path $packageManifest -Description "deployment package manifest"

Assert-PowerShellSyntax -Path $startBackendScript -Description "start backend script"
Assert-PowerShellSyntax -Path $packageSmokeScript -Description "package smoke script"

Validate-BackendConfiguration -Path $backendConfig
Validate-FrontendConfiguration -Path $frontendConfig
Validate-PackageManifest -Path $packageManifest

$requiredDocs = @(
    "http-api-contracts-v1.md",
    "blazor-local-development.md",
    "sqlite-storage-v1.md",
    "modbus-tcp-adapter-v1.md",
    "notification-delivery-v1.md",
    "auth-audit-v1.md",
    "release-v1.7.0-auth-audit.md",
    "deployment-packaging-v1.md",
    "release-v1.8.0-service-packaging.md"
)

foreach ($doc in $requiredDocs) {
    Assert-FileNotEmpty -Path (Join-Path $docsDirectory $doc) -Description "documentation $doc"
}

if (-not $SkipFrontendChecks.IsPresent) {
    $indexHtml = Join-Path $wwwrootDirectory "index.html"
    $frameworkDirectory = Join-Path $wwwrootDirectory "_framework"
    $publishedAppsettings = Join-Path $wwwrootDirectory "appsettings.json"

    Assert-FileNotEmpty -Path $indexHtml -Description "frontend index.html"
    Assert-PathExists -Path $frameworkDirectory -Description "frontend _framework directory"
    Assert-FileNotEmpty -Path $publishedAppsettings -Description "frontend published appsettings.json"

    $frameworkFiles = Get-ChildItem -Path $frameworkDirectory -Recurse -File

    if ($frameworkFiles.Count -le 0) {
        throw "frontend _framework directory does not contain files: $frameworkDirectory"
    }

    Write-Host "[OK] frontend _framework contains files"

    Validate-FrontendConfiguration -Path $publishedAppsettings
}
else {
    Write-Host "Skipping frontend checks."
}

$backendProcess = $null

try {
    if (-not $SkipE2ESmoke.IsPresent) {
        Write-Host "Running package e2e smoke: $e2eSmokeExecutable $E2ESampleCount"

        & $e2eSmokeExecutable $E2ESampleCount

        if ($LASTEXITCODE -ne 0) {
            throw "Package e2e smoke failed with exit code $LASTEXITCODE."
        }

        Write-Host "[OK] package e2e smoke"
    }
    else {
        Write-Host "Skipping package e2e smoke."
    }

    if (-not $SkipEndpointChecks.IsPresent) {
        $baseUrl = "http://$HostAddress`:$Port"

        if ($StartBackend.IsPresent) {
            Write-Host "Starting packaged backend for endpoint checks."

            $backendProcess = & $startBackendScript `
                -PackageRoot $resolvedPackageRoot `
                -HostAddress $HostAddress `
                -Port $Port `
                -PassThru

            if ($StartupDelaySeconds -gt 0) {
                Start-Sleep -Seconds $StartupDelaySeconds
            }

            Wait-ForBackendReady `
                -BaseUrl $baseUrl `
                -TimeoutSeconds $BackendReadyTimeoutSeconds
        }
        else {
            Write-Host "Endpoint checks will use an already running backend at $baseUrl."
            Wait-ForBackendReady `
                -BaseUrl $baseUrl `
                -TimeoutSeconds $BackendReadyTimeoutSeconds
        }

        Invoke-EndpointCheck -Uri "$baseUrl/health" -Description "health endpoint"
        Invoke-EndpointCheck -Uri "$baseUrl/ready" -Description "ready endpoint"
        Invoke-EndpointCheck -Uri "$baseUrl/api/v1/runtime" -Description "runtime endpoint"
        Invoke-EndpointCheck -Uri "$baseUrl/api/v1/alarms" -Description "alarms endpoint"
    }
    else {
        Write-Host "Skipping endpoint checks."
    }
}
finally {
    if ($null -ne $backendProcess) {
        Write-Host "Stopping Dispatcher backend process. PID: $($backendProcess.Id)"

        Stop-Process -Id $backendProcess.Id -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "Dispatcher package smoke validation completed successfully."

