[CmdletBinding()]
param(
    [string]$PackageRoot = "",
    [string]$RepositoryRoot = "",
    [string]$HostAddress = "127.0.0.1",
    [int]$Port = 18080,
    [switch]$UseRepositoryBuild,
    [switch]$Wait,
    [switch]$PassThru
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

function Resolve-PackageRoot {
    param(
        [string]$ExplicitPackageRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitPackageRoot)) {
        return (Resolve-Path $ExplicitPackageRoot).Path
    }

    $scriptDirectory = Split-Path -Parent $PSCommandPath
    $candidatePackageRoot = Split-Path -Parent $scriptDirectory
    $candidateBinary = Join-Path $candidatePackageRoot "bin\dispatcher-http-server.exe"

    if (Test-Path $candidateBinary) {
        return (Resolve-Path $candidatePackageRoot).Path
    }

    return ""
}

function Resolve-BackendExecutable {
    param(
        [string]$ExplicitPackageRoot,
        [string]$ExplicitRepositoryRoot,
        [bool]$PreferRepositoryBuild
    )

    if (-not $PreferRepositoryBuild) {
        $resolvedPackageRoot = Resolve-PackageRoot -ExplicitPackageRoot $ExplicitPackageRoot

        if (-not [string]::IsNullOrWhiteSpace($resolvedPackageRoot)) {
            $packageBinary = Join-Path $resolvedPackageRoot "bin\dispatcher-http-server.exe"

            if (Test-Path $packageBinary) {
                return @{
                    Path = (Resolve-Path $packageBinary).Path
                    WorkingDirectory = $resolvedPackageRoot
                    Mode = "package"
                }
            }

            throw "Package backend executable was not found: $packageBinary"
        }
    }

    $resolvedRepositoryRoot = Resolve-RepositoryRoot -ExplicitRepositoryRoot $ExplicitRepositoryRoot
    $repositoryBinary = Join-Path $resolvedRepositoryRoot "out\build\windows-vs-debug\backend\apps\dispatcher-http-server\Debug\dispatcher-http-server.exe"

    if (-not (Test-Path $repositoryBinary)) {
        throw "Repository backend executable was not found. Build the backend first: $repositoryBinary"
    }

    return @{
        Path = (Resolve-Path $repositoryBinary).Path
        WorkingDirectory = $resolvedRepositoryRoot
        Mode = "repository-build"
    }
}

if ($Port -le 0) {
    throw "Port must be greater than zero."
}

$resolved = Resolve-BackendExecutable `
    -ExplicitPackageRoot $PackageRoot `
    -ExplicitRepositoryRoot $RepositoryRoot `
    -PreferRepositoryBuild $UseRepositoryBuild.IsPresent

$arguments = @(
    $HostAddress,
    [string]$Port
)

Write-Host "Starting Dispatcher backend"
Write-Host "Mode: $($resolved.Mode)"
Write-Host "Executable: $($resolved.Path)"
Write-Host "Working directory: $($resolved.WorkingDirectory)"
Write-Host "Address: $HostAddress`:$Port"

if ($Wait.IsPresent) {
    Push-Location $resolved.WorkingDirectory

    try {
        & $resolved.Path @arguments
    }
    finally {
        Pop-Location
    }

    return
}

$process = Start-Process `
    -FilePath $resolved.Path `
    -ArgumentList $arguments `
    -WorkingDirectory $resolved.WorkingDirectory `
    -PassThru

Write-Host "Dispatcher backend process started. PID: $($process.Id)"

if ($PassThru.IsPresent) {
    return $process
}