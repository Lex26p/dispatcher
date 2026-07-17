[CmdletBinding()]
param(
    [string]$RepositoryRoot = "",
    [string]$ManifestPath = "deployment\package-layout.json",
    [string]$PackageRoot = "",
    [string]$BackendPreset = "windows-vs-debug",
    [string]$FrontendConfiguration = "Release",
    [switch]$Clean,
    [switch]$SkipBackendBuild,
    [switch]$SkipFrontendPublish,
    [switch]$CreateZip
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

function Invoke-NativeCommand {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$Description,
        [string]$WorkingDirectory
    )

    Write-Host $Description
    Write-Host "Command: $FilePath $($Arguments -join ' ')"

    Push-Location $WorkingDirectory

    try {
        & $FilePath @Arguments

        if ($LASTEXITCODE -ne 0) {
            throw "$Description failed with exit code $LASTEXITCODE."
        }
    }
    finally {
        Pop-Location
    }

    Write-Host "[OK] $Description"
}

function Copy-RequiredFile {
    param(
        [string]$Source,
        [string]$Destination,
        [string]$Description
    )

    Assert-PathExists -Path $Source -Description "$Description source"

    $destinationDirectory = Split-Path -Parent $Destination

    if (-not [string]::IsNullOrWhiteSpace($destinationDirectory)) {
        New-Item -ItemType Directory -Force $destinationDirectory | Out-Null
    }

    Copy-Item -Path $Source -Destination $Destination -Force

    Assert-PathExists -Path $Destination -Description "$Description destination"

    Write-Host "[OK] copied $Description"
}

function Copy-RuntimeDllsFromDirectory {
    param(
        [string]$SourceDirectory,
        [string]$DestinationDirectory,
        [string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($SourceDirectory)) {
        return
    }

    if (-not (Test-Path $SourceDirectory)) {
        return
    }

    $dlls = Get-ChildItem -Path $SourceDirectory -Filter "*.dll" -File -ErrorAction SilentlyContinue

    foreach ($dll in $dlls) {
        $destination = Join-Path $DestinationDirectory $dll.Name

        Copy-Item `
            -Path $dll.FullName `
            -Destination $destination `
            -Force

        Write-Host "[OK] copied runtime DLL $($dll.Name) from $Description"
    }
}

function Copy-RuntimeDependenciesForBinary {
    param(
        [string]$RepositoryRoot,
        [string]$BackendPreset,
        [string]$BinarySourcePath,
        [string]$BinaryDestinationPath
    )

    $destinationDirectory = Split-Path -Parent $BinaryDestinationPath
    $binarySourceDirectory = Split-Path -Parent $BinarySourcePath

    New-Item -ItemType Directory -Force $destinationDirectory | Out-Null

    Copy-RuntimeDllsFromDirectory `
        -SourceDirectory $binarySourceDirectory `
        -DestinationDirectory $destinationDirectory `
        -Description "binary output directory"

    $candidateDirectories = @(
        (Join-Path $RepositoryRoot "out\build\$BackendPreset\vcpkg_installed\x64-windows\debug\bin"),
        (Join-Path $RepositoryRoot "out\build\$BackendPreset\vcpkg_installed\x64-windows\bin"),
        (Join-Path $RepositoryRoot "vcpkg_installed\x64-windows\debug\bin"),
        (Join-Path $RepositoryRoot "vcpkg_installed\x64-windows\bin")
    )

    foreach ($candidateDirectory in $candidateDirectories) {
        Copy-RuntimeDllsFromDirectory `
            -SourceDirectory $candidateDirectory `
            -DestinationDirectory $destinationDirectory `
            -Description $candidateDirectory
    }
}

function Copy-DirectoryContents {
    param(
        [string]$Source,
        [string]$Destination,
        [string]$Description
    )

    Assert-PathExists -Path $Source -Description "$Description source"

    New-Item -ItemType Directory -Force $Destination | Out-Null

    Copy-Item `
        -Path (Join-Path $Source "*") `
        -Destination $Destination `
        -Recurse `
        -Force

    Write-Host "[OK] copied $Description"
}

function Get-GitCommit {
    param(
        [string]$Root
    )

    Push-Location $Root

    try {
        $commit = git rev-parse --short HEAD 2>$null

        if ($LASTEXITCODE -ne 0) {
            return ""
        }

        return $commit
    }
    catch {
        return ""
    }
    finally {
        Pop-Location
    }
}

Assert-StringNotEmpty -Value $ManifestPath -Description "ManifestPath"
Assert-StringNotEmpty -Value $BackendPreset -Description "BackendPreset"
Assert-StringNotEmpty -Value $FrontendConfiguration -Description "FrontendConfiguration"

$root = Resolve-RepositoryRoot -ExplicitRepositoryRoot $RepositoryRoot
$manifestFullPath = Resolve-PathFromRoot -Root $root -Path $ManifestPath

Assert-PathExists -Path $manifestFullPath -Description "deployment package layout manifest"

$manifest = Read-JsonFile -Path $manifestFullPath -Description "deployment package layout manifest"

if ($manifest.schemaVersion -ne 1) {
    throw "Unsupported package layout schemaVersion: $($manifest.schemaVersion)"
}

Assert-StringNotEmpty -Value $manifest.packageName -Description "manifest packageName"
Assert-StringNotEmpty -Value $manifest.versionTag -Description "manifest versionTag"
Assert-StringNotEmpty -Value $manifest.packageRoot -Description "manifest packageRoot"

$packageRootValue = $PackageRoot

if ([string]::IsNullOrWhiteSpace($packageRootValue)) {
    $packageRootValue = $manifest.packageRoot
}

$packageRootFullPath = Resolve-PathFromRoot -Root $root -Path $packageRootValue

Write-Host "Dispatcher package build"
Write-Host "Repository root: $root"
Write-Host "Manifest: $manifestFullPath"
Write-Host "Package root: $packageRootFullPath"
Write-Host "Backend preset: $BackendPreset"
Write-Host "Frontend configuration: $FrontendConfiguration"

if ($Clean.IsPresent -and (Test-Path $packageRootFullPath)) {
    Write-Host "Cleaning package root: $packageRootFullPath"

    Remove-Item -Path $packageRootFullPath -Recurse -Force
}

New-Item -ItemType Directory -Force $packageRootFullPath | Out-Null

if (-not $SkipBackendBuild.IsPresent) {
    Invoke-NativeCommand `
        -FilePath "cmake" `
        -Arguments @("--preset", $BackendPreset) `
        -Description "backend configure" `
        -WorkingDirectory $root

    Invoke-NativeCommand `
        -FilePath "cmake" `
        -Arguments @("--build", "--preset", $BackendPreset) `
        -Description "backend build" `
        -WorkingDirectory $root
}
else {
    Write-Host "Skipping backend build."
}

$frontendPublishSourceFullPath = Resolve-PathFromRoot -Root $root -Path $manifest.frontend.publishSource
$frontendPublishRootRelative = Split-Path -Parent $manifest.frontend.publishSource
$frontendPublishScript = Join-Path $root "scripts\deployment\Publish-DispatcherFrontend.ps1"

Assert-PathExists -Path $frontendPublishScript -Description "frontend publish script"

if (-not $SkipFrontendPublish.IsPresent) {
    Write-Host "Publishing frontend"

    & $frontendPublishScript `
        -RepositoryRoot $root `
        -Configuration $FrontendConfiguration `
        -PublishRoot $frontendPublishRootRelative `
        -Clean

    if ($LASTEXITCODE -ne 0) {
        throw "Frontend publish script failed with exit code $LASTEXITCODE."
    }
}
else {
    Write-Host "Skipping frontend publish."
}

Assert-PathExists -Path $frontendPublishSourceFullPath -Description "frontend publish source"

foreach ($directory in @($manifest.requiredDirectories)) {
    Assert-StringNotEmpty -Value $directory -Description "required package directory entry"

    $directoryPath = Join-Path $packageRootFullPath $directory

    New-Item -ItemType Directory -Force $directoryPath | Out-Null

    Write-Host "[OK] package directory $directory"
}

foreach ($binary in @($manifest.backendBinaries)) {
    Assert-StringNotEmpty -Value $binary.name -Description "backend binary name"
    Assert-StringNotEmpty -Value $binary.source -Description "backend binary source for $($binary.name)"
    Assert-StringNotEmpty -Value $binary.destination -Description "backend binary destination for $($binary.name)"

    if ($binary.required -eq $true) {
        $sourcePath = Resolve-PathFromRoot -Root $root -Path $binary.source
        $destinationPath = Join-Path $packageRootFullPath $binary.destination

        Copy-RequiredFile `
            -Source $sourcePath `
            -Destination $destinationPath `
            -Description "backend binary $($binary.name)"

        Copy-RuntimeDependenciesForBinary `
            -RepositoryRoot $root `
            -BackendPreset $BackendPreset `
            -BinarySourcePath $sourcePath `
            -BinaryDestinationPath $destinationPath
    }
}

foreach ($configurationFile in @($manifest.configurationFiles)) {
    Assert-StringNotEmpty -Value $configurationFile.name -Description "configuration file name"
    Assert-StringNotEmpty -Value $configurationFile.source -Description "configuration file source for $($configurationFile.name)"
    Assert-StringNotEmpty -Value $configurationFile.destination -Description "configuration file destination for $($configurationFile.name)"

    if ($configurationFile.required -eq $true) {
        $sourcePath = Resolve-PathFromRoot -Root $root -Path $configurationFile.source
        $destinationPath = Join-Path $packageRootFullPath $configurationFile.destination

        Copy-RequiredFile `
            -Source $sourcePath `
            -Destination $destinationPath `
            -Description "configuration file $($configurationFile.name)"
    }
}

foreach ($script in @($manifest.scripts)) {
    Assert-StringNotEmpty -Value $script.name -Description "script name"
    Assert-StringNotEmpty -Value $script.source -Description "script source for $($script.name)"
    Assert-StringNotEmpty -Value $script.destination -Description "script destination for $($script.name)"

    if ($script.required -eq $true) {
        $sourcePath = Resolve-PathFromRoot -Root $root -Path $script.source
        $destinationPath = Join-Path $packageRootFullPath $script.destination

        Copy-RequiredFile `
            -Source $sourcePath `
            -Destination $destinationPath `
            -Description "deployment script $($script.name)"
    }
}

$frontendDestination = Join-Path $packageRootFullPath $manifest.frontend.destination

Copy-DirectoryContents `
    -Source $frontendPublishSourceFullPath `
    -Destination $frontendDestination `
    -Description "frontend wwwroot"

foreach ($document in @($manifest.documentation)) {
    Assert-StringNotEmpty -Value $document.source -Description "documentation source"
    Assert-StringNotEmpty -Value $document.destination -Description "documentation destination"

    if ($document.required -eq $true) {
        $sourcePath = Resolve-PathFromRoot -Root $root -Path $document.source
        $destinationPath = Join-Path $packageRootFullPath $document.destination

        Copy-RequiredFile `
            -Source $sourcePath `
            -Destination $destinationPath `
            -Description "documentation $($document.source)"
    }
}

$packageFileCount = @(Get-ChildItem -Path $packageRootFullPath -Recurse -File).Count
$packageDirectoryCount = @(Get-ChildItem -Path $packageRootFullPath -Recurse -Directory).Count
$gitCommit = Get-GitCommit -Root $root

$packageManifestPath = Join-Path $packageRootFullPath "deployment-package.json"

$packageManifest = [PSCustomObject]@{
    schemaVersion = 1
    packageName = $manifest.packageName
    versionTag = $manifest.versionTag
    packageRoot = $packageRootValue
    backendPreset = $BackendPreset
    frontendConfiguration = $FrontendConfiguration
    gitCommit = $gitCommit
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
    directories = @($manifest.requiredDirectories)
    backendBinaries = @($manifest.backendBinaries | ForEach-Object {
        [PSCustomObject]@{
            name = $_.name
            destination = $_.destination
            required = $_.required
        }
    })
    frontend = [PSCustomObject]@{
        publishSource = $manifest.frontend.publishSource
        destination = $manifest.frontend.destination
    }
    fileCount = $packageFileCount
    directoryCount = $packageDirectoryCount
}

$packageManifest `
    | ConvertTo-Json -Depth 20 `
    | Set-Content -Path $packageManifestPath -Encoding UTF8

Assert-PathExists -Path $packageManifestPath -Description "deployment package manifest"

Write-Host "[OK] deployment package manifest created"

$zipPath = ""

if ($CreateZip.IsPresent) {
    $packageParent = Split-Path -Parent $packageRootFullPath
    $zipName = "$($manifest.packageName)-$($manifest.versionTag).zip"
    $zipPath = Join-Path $packageParent $zipName

    if (Test-Path $zipPath) {
        Remove-Item -Path $zipPath -Force
    }

    Write-Host "Creating package archive: $zipPath"

    Compress-Archive `
        -Path (Join-Path $packageRootFullPath "*") `
        -DestinationPath $zipPath `
        -Force

    Assert-PathExists -Path $zipPath -Description "deployment package archive"

    Write-Host "[OK] package archive created"
}

Write-Host ""
Write-Host "Dispatcher deployment package build completed successfully."
Write-Host "Package root: $packageRootFullPath"

if (-not [string]::IsNullOrWhiteSpace($zipPath)) {
    Write-Host "Package archive: $zipPath"
}