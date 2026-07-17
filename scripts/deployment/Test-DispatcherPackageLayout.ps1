[CmdletBinding()]
param(
    [string]$RepositoryRoot = ""
)

$ErrorActionPreference = "Stop"

function Resolve-RepositoryRoot {
    param(
        [string]$ExplicitRepositoryRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitRepositoryRoot)) {
        return (Resolve-Path $ExplicitRepositoryRoot).Path
    }

    $scriptDirectory = Split-Path -Parent $PSCommandPath
    $scriptsDirectory = Split-Path -Parent $scriptDirectory
    $repositoryRoot = Split-Path -Parent $scriptsDirectory

    return (Resolve-Path $repositoryRoot).Path
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

function Assert-ArrayNotEmpty {
    param(
        [object[]]$Value,
        [string]$Description
    )

    if ($null -eq $Value -or @($Value).Count -eq 0) {
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

function Assert-ObjectPropertyExists {
    param(
        [object]$Object,
        [string]$PropertyName,
        [string]$Description
    )

    $property = $Object.PSObject.Properties[$PropertyName]

    if ($null -eq $property) {
        throw "Missing property $PropertyName in $Description"
    }

    Write-Host "[OK] $Description.$PropertyName"
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

function Validate-BackendRuntimeTemplate {
    param(
        [string]$Path
    )

    $template = Read-JsonFile -Path $Path -Description "backend runtime template"

    Assert-JsonSchemaVersion -Json $template -ExpectedSchemaVersion 1 -Description "backend runtime template"

    Assert-ObjectPropertyExists -Object $template -PropertyName "application" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "server" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "runtime" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "storage" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "telemetry" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "notifications" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "authAudit" -Description "backend runtime template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "logging" -Description "backend runtime template"

    Assert-StringNotEmpty -Value $template.application.name -Description "backend runtime template application.name"
    Assert-StringNotEmpty -Value $template.application.component -Description "backend runtime template application.component"
    Assert-StringNotEmpty -Value $template.application.environment -Description "backend runtime template application.environment"
    Assert-StringNotEmpty -Value $template.application.versionTag -Description "backend runtime template application.versionTag"

    Assert-StringNotEmpty -Value $template.server.host -Description "backend runtime template server.host"

    if ($template.server.port -le 0) {
        throw "backend runtime template server.port must be greater than zero"
    }

    Write-Host "[OK] backend runtime template server.port"

    Assert-StringNotEmpty -Value $template.server.baseUrl -Description "backend runtime template server.baseUrl"
    Assert-StringNotEmpty -Value $template.storage.provider -Description "backend runtime template storage.provider"
    Assert-StringNotEmpty -Value $template.storage.sqlite.databasePath -Description "backend runtime template storage.sqlite.databasePath"
    Assert-StringNotEmpty -Value $template.telemetry.provider -Description "backend runtime template telemetry.provider"
    Assert-StringNotEmpty -Value $template.logging.directory -Description "backend runtime template logging.directory"
}

function Validate-FrontendAppsettingsTemplate {
    param(
        [string]$Path
    )

    $template = Read-JsonFile -Path $Path -Description "frontend appsettings template"

    Assert-JsonSchemaVersion -Json $template -ExpectedSchemaVersion 1 -Description "frontend appsettings template"

    Assert-ObjectPropertyExists -Object $template -PropertyName "application" -Description "frontend appsettings template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "DispatcherApi" -Description "frontend appsettings template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "Features" -Description "frontend appsettings template"
    Assert-ObjectPropertyExists -Object $template -PropertyName "Deployment" -Description "frontend appsettings template"

    Assert-StringNotEmpty -Value $template.application.name -Description "frontend appsettings template application.name"
    Assert-StringNotEmpty -Value $template.application.component -Description "frontend appsettings template application.component"
    Assert-StringNotEmpty -Value $template.application.environment -Description "frontend appsettings template application.environment"
    Assert-StringNotEmpty -Value $template.application.versionTag -Description "frontend appsettings template application.versionTag"

    Assert-StringNotEmpty -Value $template.DispatcherApi.BaseUrl -Description "frontend appsettings template DispatcherApi.BaseUrl"
    Assert-StringNotEmpty -Value $template.DispatcherApi.HealthEndpoint -Description "frontend appsettings template DispatcherApi.HealthEndpoint"
    Assert-StringNotEmpty -Value $template.DispatcherApi.ReadinessEndpoint -Description "frontend appsettings template DispatcherApi.ReadinessEndpoint"
    Assert-StringNotEmpty -Value $template.DispatcherApi.RuntimeEndpoint -Description "frontend appsettings template DispatcherApi.RuntimeEndpoint"
    Assert-StringNotEmpty -Value $template.DispatcherApi.AlarmsEndpoint -Description "frontend appsettings template DispatcherApi.AlarmsEndpoint"

    if ($template.DispatcherApi.TimeoutSeconds -le 0) {
        throw "frontend appsettings template DispatcherApi.TimeoutSeconds must be greater than zero"
    }

    Write-Host "[OK] frontend appsettings template DispatcherApi.TimeoutSeconds"
}

$root = Resolve-RepositoryRoot -ExplicitRepositoryRoot $RepositoryRoot

Write-Host "Dispatcher package layout validation"
Write-Host "Repository root: $root"

Assert-PathExists -Path (Join-Path $root "backend") -Description "backend directory"
Assert-PathExists -Path (Join-Path $root "frontend") -Description "frontend directory"
Assert-PathExists -Path (Join-Path $root "docs") -Description "docs directory"
Assert-PathExists -Path (Join-Path $root "deployment") -Description "deployment directory"
Assert-PathExists -Path (Join-Path $root "deployment\templates") -Description "deployment templates directory"
Assert-PathExists -Path (Join-Path $root "scripts") -Description "scripts directory"
Assert-PathExists -Path (Join-Path $root "scripts\deployment") -Description "deployment scripts directory"

$manifestPath = Join-Path $root "deployment\package-layout.json"

Assert-PathExists -Path $manifestPath -Description "deployment package layout manifest"

$manifest = Read-JsonFile -Path $manifestPath -Description "deployment package layout manifest"

Assert-StringNotEmpty -Value $manifest.packageName -Description "manifest packageName"
Assert-StringNotEmpty -Value $manifest.versionTag -Description "manifest versionTag"
Assert-StringNotEmpty -Value $manifest.packageRoot -Description "manifest packageRoot"
Assert-ArrayNotEmpty -Value @($manifest.requiredDirectories) -Description "manifest requiredDirectories"
Assert-ArrayNotEmpty -Value @($manifest.backendBinaries) -Description "manifest backendBinaries"
Assert-ArrayNotEmpty -Value @($manifest.configurationFiles) -Description "manifest configurationFiles"
Assert-ArrayNotEmpty -Value @($manifest.scripts) -Description "manifest scripts"
Assert-ArrayNotEmpty -Value @($manifest.documentation) -Description "manifest documentation"
Assert-ArrayNotEmpty -Value @($manifest.smokeChecks) -Description "manifest smokeChecks"

Assert-JsonSchemaVersion -Json $manifest -ExpectedSchemaVersion 1 -Description "deployment package layout manifest"

foreach ($directory in @($manifest.requiredDirectories)) {
    Assert-StringNotEmpty -Value $directory -Description "required package directory entry"
}

foreach ($binary in @($manifest.backendBinaries)) {
    Assert-StringNotEmpty -Value $binary.name -Description "backend binary name"
    Assert-StringNotEmpty -Value $binary.source -Description "backend binary source for $($binary.name)"
    Assert-StringNotEmpty -Value $binary.destination -Description "backend binary destination for $($binary.name)"
}

Assert-ObjectPropertyExists -Object $manifest -PropertyName "frontend" -Description "manifest"
Assert-StringNotEmpty -Value $manifest.frontend.project -Description "frontend project"
Assert-StringNotEmpty -Value $manifest.frontend.publishSource -Description "frontend publishSource"
Assert-StringNotEmpty -Value $manifest.frontend.destination -Description "frontend destination"

Assert-PathExists -Path (Join-Path $root $manifest.frontend.project) -Description "frontend project file"

foreach ($configurationFile in @($manifest.configurationFiles)) {
    Assert-StringNotEmpty -Value $configurationFile.name -Description "configuration file name"
    Assert-StringNotEmpty -Value $configurationFile.source -Description "configuration file source for $($configurationFile.name)"
    Assert-StringNotEmpty -Value $configurationFile.destination -Description "configuration file destination for $($configurationFile.name)"

    if ($configurationFile.required -eq $true) {
        $configurationSourcePath = Join-Path $root $configurationFile.source

        Assert-PathExists -Path $configurationSourcePath -Description "required configuration template $($configurationFile.source)"

        $configJson = Read-JsonFile -Path $configurationSourcePath -Description "configuration template $($configurationFile.name)"

        Assert-JsonSchemaVersion -Json $configJson -ExpectedSchemaVersion 1 -Description "configuration template $($configurationFile.name)"
    }
}

$backendRuntimeTemplatePath = Join-Path $root "deployment\templates\backend-runtime.json"
$frontendAppsettingsTemplatePath = Join-Path $root "deployment\templates\frontend-appsettings.json"

Assert-PathExists -Path $backendRuntimeTemplatePath -Description "backend runtime configuration template"
Assert-PathExists -Path $frontendAppsettingsTemplatePath -Description "frontend appsettings configuration template"

Validate-BackendRuntimeTemplate -Path $backendRuntimeTemplatePath
Validate-FrontendAppsettingsTemplate -Path $frontendAppsettingsTemplatePath

foreach ($script in @($manifest.scripts)) {
    Assert-StringNotEmpty -Value $script.name -Description "script name"
    Assert-StringNotEmpty -Value $script.source -Description "script source for $($script.name)"
    Assert-StringNotEmpty -Value $script.destination -Description "script destination for $($script.name)"

    if ($script.required -eq $true) {
        $scriptSourcePath = Join-Path $root $script.source

        Assert-PathExists -Path $scriptSourcePath -Description "required deployment script $($script.source)"
        Assert-PowerShellSyntax -Path $scriptSourcePath -Description "required deployment script $($script.source)"
    }
}

$startBackendScriptPath = Join-Path $root "scripts\deployment\Start-DispatcherBackend.ps1"
$testPackageScriptPath = Join-Path $root "scripts\deployment\Test-DispatcherPackage.ps1"
$publishFrontendScriptPath = Join-Path $root "scripts\deployment\Publish-DispatcherFrontend.ps1"
$testFrontendPublishScriptPath = Join-Path $root "scripts\deployment\Test-DispatcherFrontendPublish.ps1"
$buildPackageScriptPath = Join-Path $root "scripts\deployment\Build-DispatcherPackage.ps1"

Assert-PathExists -Path $startBackendScriptPath -Description "start backend helper script"
Assert-PathExists -Path $testPackageScriptPath -Description "package smoke helper script"
Assert-PathExists -Path $publishFrontendScriptPath -Description "frontend publish helper script"
Assert-PathExists -Path $testFrontendPublishScriptPath -Description "frontend publish validation helper script"
Assert-PathExists -Path $buildPackageScriptPath -Description "package build helper script"

Assert-PowerShellSyntax -Path $startBackendScriptPath -Description "start backend helper script"
Assert-PowerShellSyntax -Path $testPackageScriptPath -Description "package smoke helper script"
Assert-PowerShellSyntax -Path $publishFrontendScriptPath -Description "frontend publish helper script"
Assert-PowerShellSyntax -Path $testFrontendPublishScriptPath -Description "frontend publish validation helper script"
Assert-PowerShellSyntax -Path $buildPackageScriptPath -Description "package build helper script"

foreach ($document in @($manifest.documentation)) {
    Assert-StringNotEmpty -Value $document.source -Description "documentation source"
    Assert-StringNotEmpty -Value $document.destination -Description "documentation destination"

    if ($document.required -eq $true) {
        Assert-PathExists -Path (Join-Path $root $document.source) -Description "required documentation $($document.source)"
    }
}

foreach ($check in @($manifest.smokeChecks)) {
    Assert-StringNotEmpty -Value $check.name -Description "smoke check name"
    Assert-StringNotEmpty -Value $check.command -Description "smoke check command for $($check.name)"
}

Assert-PathExists -Path (Join-Path $root "backend\apps\dispatcher-http-server") -Description "dispatcher-http-server app source"
Assert-PathExists -Path (Join-Path $root "backend\apps\dispatcher-e2e-smoke") -Description "dispatcher-e2e-smoke app source"
Assert-PathExists -Path (Join-Path $root "docs\release\v1.7.0-auth-audit.md") -Description "latest release document"

Write-Host ""
Write-Host "Package layout contract validation completed successfully."