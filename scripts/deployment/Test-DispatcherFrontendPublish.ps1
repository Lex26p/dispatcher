[CmdletBinding()]
param(
    [string]$RepositoryRoot = "",
    [string]$PublishRoot = "artifacts\frontend\publish"
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

function Resolve-PathFromRoot {
    param(
        [string]$Root,
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $Root $Path
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

$root = Resolve-RepositoryRoot -ExplicitRepositoryRoot $RepositoryRoot
$publishRootFullPath = Resolve-PathFromRoot -Root $root -Path $PublishRoot

Write-Host "Dispatcher frontend publish validation"
Write-Host "Repository root: $root"
Write-Host "Publish root: $publishRootFullPath"

Assert-PathExists -Path $publishRootFullPath -Description "frontend publish root"

$wwwroot = Join-Path $publishRootFullPath "wwwroot"
$indexHtml = Join-Path $wwwroot "index.html"
$frameworkDirectory = Join-Path $wwwroot "_framework"
$appsettings = Join-Path $wwwroot "appsettings.json"
$publishManifest = Join-Path $publishRootFullPath "dispatcher-frontend-publish.json"

Assert-PathExists -Path $wwwroot -Description "published wwwroot directory"
Assert-FileNotEmpty -Path $indexHtml -Description "published index.html"
Assert-PathExists -Path $frameworkDirectory -Description "published _framework directory"
Assert-FileNotEmpty -Path $appsettings -Description "published appsettings.json"
Assert-FileNotEmpty -Path $publishManifest -Description "frontend publish manifest"

$frameworkFiles = Get-ChildItem -Path $frameworkDirectory -Recurse -File

if ($frameworkFiles.Count -le 0) {
    throw "Published _framework directory does not contain files: $frameworkDirectory"
}

Write-Host "[OK] published _framework contains files"

$appsettingsJson = Read-JsonFile -Path $appsettings -Description "published appsettings.json"

if ($appsettingsJson.schemaVersion -ne 1) {
    throw "Unsupported published appsettings schemaVersion: $($appsettingsJson.schemaVersion)"
}

Assert-StringNotEmpty -Value $appsettingsJson.DispatcherApi.BaseUrl -Description "published appsettings DispatcherApi.BaseUrl"
Assert-StringNotEmpty -Value $appsettingsJson.DispatcherApi.HealthEndpoint -Description "published appsettings DispatcherApi.HealthEndpoint"
Assert-StringNotEmpty -Value $appsettingsJson.DispatcherApi.ReadinessEndpoint -Description "published appsettings DispatcherApi.ReadinessEndpoint"
Assert-StringNotEmpty -Value $appsettingsJson.DispatcherApi.RuntimeEndpoint -Description "published appsettings DispatcherApi.RuntimeEndpoint"
Assert-StringNotEmpty -Value $appsettingsJson.DispatcherApi.AlarmsEndpoint -Description "published appsettings DispatcherApi.AlarmsEndpoint"

if ($appsettingsJson.DispatcherApi.TimeoutSeconds -le 0) {
    throw "published appsettings DispatcherApi.TimeoutSeconds must be greater than zero"
}

Write-Host "[OK] published appsettings DispatcherApi.TimeoutSeconds"

$manifestJson = Read-JsonFile -Path $publishManifest -Description "frontend publish manifest"

if ($manifestJson.schemaVersion -ne 1) {
    throw "Unsupported frontend publish manifest schemaVersion: $($manifestJson.schemaVersion)"
}

Assert-StringNotEmpty -Value $manifestJson.component -Description "frontend publish manifest component"
Assert-StringNotEmpty -Value $manifestJson.project -Description "frontend publish manifest project"
Assert-StringNotEmpty -Value $manifestJson.configuration -Description "frontend publish manifest configuration"
Assert-StringNotEmpty -Value $manifestJson.publishRoot -Description "frontend publish manifest publishRoot"
Assert-StringNotEmpty -Value $manifestJson.wwwroot -Description "frontend publish manifest wwwroot"
Assert-StringNotEmpty -Value $manifestJson.generatedAtUtc -Description "frontend publish manifest generatedAtUtc"

Write-Host ""
Write-Host "Dispatcher frontend publish validation completed successfully."