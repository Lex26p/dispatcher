[CmdletBinding()]
param(
    [string]$RepositoryRoot = "",
    [string]$Configuration = "Release",
    [string]$ProjectPath = "frontend\Dispatcher.Web\Dispatcher.Web.csproj",
    [string]$PublishRoot = "artifacts\frontend\publish",
    [string]$FrontendAppsettingsTemplate = "deployment\templates\frontend-appsettings.json",
    [switch]$Clean
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

Assert-StringNotEmpty -Value $Configuration -Description "Configuration"
Assert-StringNotEmpty -Value $ProjectPath -Description "ProjectPath"
Assert-StringNotEmpty -Value $PublishRoot -Description "PublishRoot"
Assert-StringNotEmpty -Value $FrontendAppsettingsTemplate -Description "FrontendAppsettingsTemplate"

$root = Resolve-RepositoryRoot -ExplicitRepositoryRoot $RepositoryRoot

$projectFullPath = Resolve-PathFromRoot -Root $root -Path $ProjectPath
$publishRootFullPath = Resolve-PathFromRoot -Root $root -Path $PublishRoot
$templateFullPath = Resolve-PathFromRoot -Root $root -Path $FrontendAppsettingsTemplate

Write-Host "Dispatcher frontend publish"
Write-Host "Repository root: $root"
Write-Host "Project: $projectFullPath"
Write-Host "Configuration: $Configuration"
Write-Host "Publish root: $publishRootFullPath"
Write-Host "Frontend appsettings template: $templateFullPath"

Assert-PathExists -Path $projectFullPath -Description "frontend project"
Assert-PathExists -Path $templateFullPath -Description "frontend appsettings template"

$templateJson = Read-JsonFile -Path $templateFullPath -Description "frontend appsettings template"

if ($templateJson.schemaVersion -ne 1) {
    throw "Unsupported frontend appsettings template schemaVersion: $($templateJson.schemaVersion)"
}

Assert-StringNotEmpty -Value $templateJson.DispatcherApi.BaseUrl -Description "frontend appsettings DispatcherApi.BaseUrl"

if ($Clean.IsPresent -and (Test-Path $publishRootFullPath)) {
    Write-Host "Cleaning frontend publish root: $publishRootFullPath"

    Remove-Item -Path $publishRootFullPath -Recurse -Force
}

New-Item -ItemType Directory -Force $publishRootFullPath | Out-Null

Push-Location $root

try {
    Write-Host "Running dotnet publish"

    dotnet publish $projectFullPath `
        -c $Configuration `
        -o $publishRootFullPath

    if ($LASTEXITCODE -ne 0) {
        throw "dotnet publish failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

$wwwroot = Join-Path $publishRootFullPath "wwwroot"

Assert-PathExists -Path $wwwroot -Description "published wwwroot directory"

$indexHtml = Join-Path $wwwroot "index.html"
$frameworkDirectory = Join-Path $wwwroot "_framework"

Assert-PathExists -Path $indexHtml -Description "published index.html"
Assert-PathExists -Path $frameworkDirectory -Description "published _framework directory"

$destinationAppsettings = Join-Path $wwwroot "appsettings.json"

Copy-Item `
    -Path $templateFullPath `
    -Destination $destinationAppsettings `
    -Force

Write-Host "[OK] copied deployment frontend appsettings to published wwwroot"

$publishedAppsettingsJson = Read-JsonFile -Path $destinationAppsettings -Description "published frontend appsettings"

if ($publishedAppsettingsJson.schemaVersion -ne 1) {
    throw "Unsupported published frontend appsettings schemaVersion: $($publishedAppsettingsJson.schemaVersion)"
}

Assert-StringNotEmpty -Value $publishedAppsettingsJson.DispatcherApi.BaseUrl -Description "published frontend appsettings DispatcherApi.BaseUrl"
Assert-StringNotEmpty -Value $publishedAppsettingsJson.DispatcherApi.HealthEndpoint -Description "published frontend appsettings DispatcherApi.HealthEndpoint"
Assert-StringNotEmpty -Value $publishedAppsettingsJson.DispatcherApi.RuntimeEndpoint -Description "published frontend appsettings DispatcherApi.RuntimeEndpoint"
Assert-StringNotEmpty -Value $publishedAppsettingsJson.DispatcherApi.AlarmsEndpoint -Description "published frontend appsettings DispatcherApi.AlarmsEndpoint"

$manifestPath = Join-Path $publishRootFullPath "dispatcher-frontend-publish.json"

$publishManifest = [PSCustomObject]@{
    schemaVersion = 1
    component = "frontend"
    project = $ProjectPath
    configuration = $Configuration
    publishRoot = $PublishRoot
    wwwroot = "wwwroot"
    appsettings = "wwwroot/appsettings.json"
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
}

$publishManifest `
    | ConvertTo-Json -Depth 10 `
    | Set-Content -Path $manifestPath -Encoding UTF8

Write-Host "[OK] frontend publish manifest created: $manifestPath"

Write-Host ""
Write-Host "Dispatcher frontend publish completed successfully."
Write-Host "Published wwwroot: $wwwroot"