[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Final')]
    [string]$Configuration,

    [string]$DeployDirectory,

    [switch]$NoDeploy
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = $PSScriptRoot
$solution = Join-Path $repoRoot 'Code\App\Src\Conquest.sln'
$outputDirectory = Join-Path $repoRoot "Code\App\Src\$Configuration"
$campaignSolution = Join-Path $repoRoot 'Code\App\Src\Scripts\ConquestCampaign.sln'
$campaignOutputDirectory = Join-Path $repoRoot 'Code\App\Src\Scripts\Build'
$engineProjects = @(
    (Join-Path $repoRoot 'Code\Libs\Src\RenderPipeline\D3DRenderPipe.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\MeshManager\MeshManager.vcxproj')
)
if ([string]::IsNullOrWhiteSpace($DeployDirectory)) {
    $DeployDirectory = if ($Configuration -eq 'Debug') {
        'D:\Games\GOG\Conquest Frontier Wars'
    }
    else {
        'D:\Games\GOG-Final\Conquest Frontier Wars'
    }
}

function Find-MSBuild {
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }
    throw 'Visual Studio 2022 MSBuild.exe was not found.'
}

function Get-Sha256([string]$Path) {
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $sha256 = [System.Security.Cryptography.SHA256]::Create()
        try {
            return ([System.BitConverter]::ToString($sha256.ComputeHash($stream))).Replace('-', '')
        }
        finally {
            $sha256.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

$msbuild = Find-MSBuild
$requiredBinaries = @('Conquest.exe', 'Globals.dll', 'Mission.dll', 'Trim.dll', 'ZBatcher.dll')
$optionalSymbols = @('Conquest.pdb', 'Globals.pdb', 'Mission.pdb', 'Trim.pdb', 'ZBatcher.pdb')
$campaignBinaries = @(
    'Mantis_T.dll'
    'SCRIPT01.dll'
    'SCRIPT02.dll'
    'SCRIPT03.dll'
    'SCRIPT04.dll'
    'SCRIPT05.dll'
    'SCRIPT06.dll'
    'SCRIPT07.dll'
    'SCRIPT08.dll'
    'SCRIPT09.dll'
    'SCRIPT10.dll'
    'SCRIPT11.dll'
    'SCRIPT12.dll'
    'SCRIPT13.dll'
    'SCRIPT14.dll'
    'SCRIPT15.dll'
    'SCRIPT16.dll'
    'Sol_T.dll'
)
$engineConfiguration = if ($Configuration -eq 'Debug') { 'debug' } else { 'release' }
$campaignConfiguration = if ($Configuration -eq 'Debug') { 'Debug' } else { 'Release' }
$dacomSource = Join-Path $repoRoot "Code\Libs\ImplicitDLL\$engineConfiguration\DACOM.dll"
$engineOutputDirectory = Join-Path $repoRoot "Code\Libs\ExplicitDLL\$engineConfiguration"
$engineBinaries = @('D3DRenderPipe.dll', 'MeshManager.dll')
if (-not $NoDeploy -and -not (Test-Path -LiteralPath $DeployDirectory -PathType Container)) {
    throw "Deployment directory does not exist: $DeployDirectory"
}

$arguments = @(
    $solution,
    '/t:Rebuild',
    '/m:1',
    "/p:Configuration=$Configuration",
    '/p:Platform=Win32',
    '/v:minimal'
)

if ($Configuration -eq 'Debug') {
    $arguments += "/p:ConquestDebugDeployDir=$DeployDirectory"
    if ($NoDeploy) {
        $arguments += '/p:ConquestEnableDeploy=false'
    }
}

Write-Host "Building engine modules: $engineConfiguration | Win32"
Write-Host "MSBuild: $msbuild"
if ($NoDeploy) {
    Write-Host 'Deployment: disabled'
}
else {
    Write-Host "Deployment: $DeployDirectory"
}

foreach ($engineProject in $engineProjects) {
    $engineArguments = @(
        $engineProject,
        '/t:Rebuild',
        '/m:1',
        "/p:Configuration=$engineConfiguration",
        '/p:Platform=Win32',
        '/v:minimal'
    )
    & $msbuild @engineArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Engine build failed for $engineProject with exit code $LASTEXITCODE."
    }
}

Write-Host "Building Conquest: $Configuration | Win32"
& $msbuild @arguments
$buildExit = $LASTEXITCODE
if ($buildExit -ne 0) {
    throw "MSBuild failed with exit code $buildExit."
}

$campaignArguments = @(
    $campaignSolution,
    '/t:Rebuild',
    '/m:1',
    "/p:Configuration=$campaignConfiguration",
    '/p:Platform=Win32',
    '/v:minimal'
)
Write-Host "Building retail campaign modules: $campaignConfiguration | Win32"
& $msbuild @campaignArguments
$campaignBuildExit = $LASTEXITCODE
if ($campaignBuildExit -ne 0) {
    throw "Campaign MSBuild failed with exit code $campaignBuildExit."
}

$missing = @($requiredBinaries | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $outputDirectory $_) -PathType Leaf)
})
if ($missing.Count -ne 0) {
    throw "Build succeeded but required outputs are missing: $($missing -join ', ')"
}
if (-not (Test-Path -LiteralPath $dacomSource -PathType Leaf)) {
    throw "Required DACOM runtime is missing: $dacomSource"
}
$missingEngine = @($engineBinaries | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $engineOutputDirectory $_) -PathType Leaf)
})
if ($missingEngine.Count -ne 0) {
    throw "Build succeeded but required engine outputs are missing: $($missingEngine -join ', ')"
}
$missingCampaign = @($campaignBinaries | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $campaignOutputDirectory $_) -PathType Leaf)
})
if ($missingCampaign.Count -ne 0) {
    throw "Campaign build succeeded but required outputs are missing: $($missingCampaign -join ', ')"
}
if (-not $NoDeploy) {
    if ($Configuration -eq 'Final') {
        foreach ($name in $requiredBinaries + $optionalSymbols) {
            $source = Join-Path $outputDirectory $name
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                Copy-Item -LiteralPath $source -Destination (Join-Path $DeployDirectory $name) -Force
            }
        }
    }

    Copy-Item -LiteralPath $dacomSource -Destination (Join-Path $DeployDirectory 'DACOM.dll') -Force

    $engineDeployDirectory = Join-Path $DeployDirectory 'DLL'
    if (-not (Test-Path -LiteralPath $engineDeployDirectory -PathType Container)) {
        New-Item -ItemType Directory -Path $engineDeployDirectory -Force | Out-Null
    }
    foreach ($name in $engineBinaries) {
        Copy-Item -LiteralPath (Join-Path $engineOutputDirectory $name) -Destination (Join-Path $engineDeployDirectory $name) -Force
    }

    $campaignDeployDirectory = Join-Path $DeployDirectory 'Scripts'
    if (-not (Test-Path -LiteralPath $campaignDeployDirectory -PathType Container)) {
        New-Item -ItemType Directory -Path $campaignDeployDirectory -Force | Out-Null
    }
    foreach ($name in $campaignBinaries) {
        Copy-Item -LiteralPath (Join-Path $campaignOutputDirectory $name) -Destination (Join-Path $campaignDeployDirectory $name) -Force
    }

    foreach ($name in $requiredBinaries) {
        $source = Join-Path $outputDirectory $name
        $deployed = Join-Path $DeployDirectory $name
        if (-not (Test-Path -LiteralPath $deployed -PathType Leaf)) {
            throw "Required deployed binary is missing: $deployed"
        }
        $sourceHash = Get-Sha256 $source
        $deployedHash = Get-Sha256 $deployed
        if ($sourceHash -ne $deployedHash) {
            throw "Deployed binary does not match the build output: $name"
        }
    }

    $deployedDacom = Join-Path $DeployDirectory 'DACOM.dll'
    if ((Get-Sha256 $dacomSource) -ne (Get-Sha256 $deployedDacom)) {
        throw 'Deployed DACOM.dll does not match the selected engine configuration.'
    }
    foreach ($name in $engineBinaries) {
        $source = Join-Path $engineOutputDirectory $name
        $deployed = Join-Path $engineDeployDirectory $name
        if ((Get-Sha256 $source) -ne (Get-Sha256 $deployed)) {
            throw "Deployed engine module does not match the build output: $name"
        }
    }
    foreach ($name in $campaignBinaries) {
        $source = Join-Path $campaignOutputDirectory $name
        $deployed = Join-Path $campaignDeployDirectory $name
        if (-not (Test-Path -LiteralPath $deployed -PathType Leaf)) {
            throw "Required deployed campaign module is missing: $deployed"
        }
        if ((Get-Sha256 $source) -ne (Get-Sha256 $deployed)) {
            throw "Deployed campaign module does not match the build output: $name"
        }
    }
}

Write-Host ''
Write-Host "Successful $Configuration build:"
Get-Item ($requiredBinaries | ForEach-Object { Join-Path $outputDirectory $_ }) |
    Select-Object Name, Length, LastWriteTime |
    Format-Table -AutoSize
Write-Host "Campaign modules: $($campaignBinaries.Count) built and verified"
if (-not $NoDeploy) {
    Write-Host "Campaign deployment: $(Join-Path $DeployDirectory 'Scripts')"
}
