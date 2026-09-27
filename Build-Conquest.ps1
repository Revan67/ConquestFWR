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

# PowerShell 7 can preserve both `Path` and `PATH` from its parent process.
# Windows itself accepts that environment block, but MSBuild's legacy tool task
# imports it into a case-insensitive dictionary and fails before launching CL.
# Collapse duplicate spellings to one canonical key so the build is reproducible
# regardless of which shell launched this wrapper.
$processEnvironment = [Environment]::GetEnvironmentVariables('Process')
$pathKeys = @($processEnvironment.Keys | Where-Object {
    [string]::Equals([string]$_, 'Path', [StringComparison]::OrdinalIgnoreCase)
})
if ($pathKeys.Count -gt 1) {
    $pathValue = [string]$processEnvironment[$pathKeys[0]]
    foreach ($pathKey in $pathKeys) {
        Remove-Item -LiteralPath "Env:$pathKey" -ErrorAction SilentlyContinue
    }
    [Environment]::SetEnvironmentVariable('Path', $pathValue, 'Process')
}

$repoRoot = $PSScriptRoot
$commonIncludeDirectory = Join-Path $repoRoot 'Code\Libs\Include'
$existingIncludePath = [Environment]::GetEnvironmentVariable('INCLUDE', 'Process')
$env:INCLUDE = if ([string]::IsNullOrWhiteSpace($existingIncludePath)) {
    $commonIncludeDirectory
}
else {
    "$commonIncludeDirectory;$existingIncludePath"
}
$solution = Join-Path $repoRoot 'Code\App\Src\Conquest.sln'
$outputDirectory = Join-Path $repoRoot "Code\App\Src\$Configuration"
$campaignSolution = Join-Path $repoRoot 'Code\App\Src\Scripts\ConquestCampaign.sln'
$campaignOutputDirectory = Join-Path $repoRoot "Code\App\Src\Scripts\Build\$Configuration"
$manifestDirectory = Join-Path $repoRoot 'Build-Manifests'
$foundationProjects = @(
    (Join-Path $repoRoot 'Code\Libs\Src\COMHeap\COMHeap.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\x86Math\MathLib.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\RenderPipeline\RPUL\RPUL.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\DACOM\DACOM.vcxproj')
)
$engineProjects = @(
    (Join-Path $repoRoot 'Code\Libs\Src\Channel\Channel.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\DAHOTKEY\Hotkey.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\Viewer\DOCUVIEW\DataViewer.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\DOSFile\DOSFile.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\Anim\Anim.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\Engine\Engine.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\RendComp\PolyMesh\PolyMesh.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\EngComps\RenderMgr\RenderMgr.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\Shading\TextureLibrary\TextureLibrary.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\TextureManager\TextureManager.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\x86Math\x86math.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\RendComp\Optics\Optics.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\EngComps\Hardpoint\HardPoint.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\Streamer\streamer.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\RenderPipeline\D3DRenderPipe.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\LightManager\LightManager.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\VertexBufferManager\VertexBufferManager.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\StringTable\StringTable.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\MaterialManager\MaterialManager.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\VideoSystem\VideoSystem.vcxproj'),
    (Join-Path $repoRoot 'Code\Libs\Src\ParticleEffect\ParticleEffect.vcxproj'),
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

function Get-BuildInputFingerprint {
    $excludedExtensions = @(
        '.dll', '.exe', '.lib', '.exp', '.pdb', '.obj', '.res', '.ilk',
        '.idb', '.ipdb', '.iobj', '.tlog', '.lastbuildstate', '.log'
    )
    $paths = @(& git -C $repoRoot ls-files -co --exclude-standard) |
        ForEach-Object { ([string]$_).Replace('\', '/') } |
        Where-Object {
            $_ -and
            $_ -notmatch '^Build-Manifests/' -and
            $_ -notmatch '^Evidence/' -and
            $_ -notmatch '^Code/Libs/(ExplicitDLL|ImplicitDLL|Static)/' -and
            $excludedExtensions -notcontains [IO.Path]::GetExtension($_).ToLowerInvariant()
        } |
        Sort-Object -Unique

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $builder = New-Object System.Text.StringBuilder
        foreach ($relativePath in $paths) {
            $absolutePath = Join-Path $repoRoot ($relativePath.Replace('/', '\'))
            if (Test-Path -LiteralPath $absolutePath -PathType Leaf) {
                [void]$builder.Append($relativePath)
                [void]$builder.Append('|')
                [void]$builder.Append((Get-Sha256 $absolutePath))
                [void]$builder.Append("`n")
            }
        }
        $bytes = [Text.Encoding]::UTF8.GetBytes($builder.ToString())
        return ([BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace('-', '')
    }
    finally {
        $sha256.Dispose()
    }
}

function Invoke-Rebuild([string]$Project, [string]$BuildConfiguration) {
    $projectArguments = @(
        $Project,
        '/t:Rebuild',
        '/m:1',
        "/p:Configuration=$BuildConfiguration",
        '/p:Platform=Win32',
        # Foundation dependencies are rebuilt explicitly above engine consumers.
        # Do not let project-reference configuration fallback rebuild Debug
        # foundations during a Release engine build.
        '/p:BuildProjectReferences=false',
        '/v:minimal'
    )
    & $msbuild @projectArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $Project with exit code $LASTEXITCODE."
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
$staticOutputDirectory = Join-Path $repoRoot "Code\Libs\Static\$engineConfiguration"
$engineBinaries = @(
    'Channel.dll',
    'DAHotkey.dll',
    'Docuview.dll',
    'DOSFile.dll',
    'Anim.dll',
    'Engine.dll',
    'PolyMesh.dll',
    'RenderMgr.dll',
    'TextureLibrary.dll',
    'TextureManager.dll',
    'x86math.dll',
    'optics.dll',
    'Hardpoint.dll',
    'Streamer.dll',
    'D3DRenderPipe.dll',
    'LightManager.dll',
    'VertexBufferManager.dll',
    'StringTable.dll',
    'MaterialManager.dll',
    'VideoSystem.dll',
    'ParticleEffect.dll',
    'MeshManager.dll'
)
if (-not $NoDeploy -and -not (Test-Path -LiteralPath $DeployDirectory -PathType Container)) {
    throw "Deployment directory does not exist: $DeployDirectory"
}

$buildStartedUtc = [DateTime]::UtcNow
$sourceRevision = (& git -C $repoRoot rev-parse HEAD).Trim()
$workingTreeFingerprint = (& git -C $repoRoot diff --binary | & git -C $repoRoot hash-object --stdin).Trim()
$workingTreeEntries = @(& git -C $repoRoot status --porcelain=v1).Count
$buildInputFingerprint = Get-BuildInputFingerprint

$arguments = @(
    $solution,
    '/t:Rebuild',
    '/m:1',
    "/p:Configuration=$Configuration",
    '/p:Platform=Win32',
    '/p:BuildProjectReferences=false',
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

Write-Host "Building foundational runtime libraries: $engineConfiguration | Win32"
foreach ($foundationProject in $foundationProjects) {
    Invoke-Rebuild $foundationProject $engineConfiguration
}

foreach ($engineProject in $engineProjects) {
    Invoke-Rebuild $engineProject $engineConfiguration
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
    "/p:ConquestCampaignOutputDirectory=$campaignOutputDirectory\",
    '/p:Platform=Win32',
    '/p:BuildProjectReferences=false',
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

$foundationArtifacts = @(
    (Join-Path $staticOutputDirectory 'COMHeap.lib'),
    (Join-Path $staticOutputDirectory 'MathLib.lib'),
    (Join-Path $staticOutputDirectory 'RPUL.lib'),
    (Join-Path $staticOutputDirectory 'DACOM.lib'),
    $dacomSource
)
$missingFoundation = @($foundationArtifacts | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) })
if ($missingFoundation.Count -ne 0) {
    throw "Build succeeded but foundational outputs are missing: $($missingFoundation -join ', ')"
}

$artifactPaths = @(
    $foundationArtifacts
    ($engineBinaries | ForEach-Object { Join-Path $engineOutputDirectory $_ })
    ($requiredBinaries | ForEach-Object { Join-Path $outputDirectory $_ })
    ($optionalSymbols | ForEach-Object {
        $path = Join-Path $outputDirectory $_
        if (Test-Path -LiteralPath $path -PathType Leaf) { $path }
    })
    ($campaignBinaries | ForEach-Object { Join-Path $campaignOutputDirectory $_ })
)
$freshnessFloor = $buildStartedUtc.AddSeconds(-5)
$staleArtifacts = @($artifactPaths | Where-Object { (Get-Item -LiteralPath $_).LastWriteTimeUtc -lt $freshnessFloor })
if ($staleArtifacts.Count -ne 0) {
    throw "Outputs were not refreshed by this rebuild: $($staleArtifacts -join ', ')"
}

$manifest = [ordered]@{
    schemaVersion = 2
    configuration = $Configuration
    engineConfiguration = $engineConfiguration
    campaignConfiguration = $campaignConfiguration
    sourceRevision = $sourceRevision
    workingTreeFingerprint = $workingTreeFingerprint
    workingTreeEntryCount = $workingTreeEntries
    buildInputFingerprint = $buildInputFingerprint
    buildStartedUtc = $buildStartedUtc.ToString('o')
    buildCompletedUtc = [DateTime]::UtcNow.ToString('o')
    msbuild = $msbuild
    artifacts = @($artifactPaths | ForEach-Object {
        $item = Get-Item -LiteralPath $_
        [ordered]@{
            path = $item.FullName.Substring($repoRoot.Length).TrimStart('\')
            bytes = $item.Length
            lastWriteUtc = $item.LastWriteTimeUtc.ToString('o')
            sha256 = Get-Sha256 $item.FullName
        }
    })
}
if (-not (Test-Path -LiteralPath $manifestDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $manifestDirectory -Force | Out-Null
}
$manifestPath = Join-Path $manifestDirectory "$Configuration-latest.json"
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8
if (-not $NoDeploy) {
    $legalNotices = @(
        'Conquest Source License.txt',
        'LEGAL.md',
        'MODIFICATIONS.md'
    )
    foreach ($name in $legalNotices) {
        $source = Join-Path $repoRoot $name
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Required legal notice is missing: $source"
        }
        Copy-Item -LiteralPath $source -Destination (Join-Path $DeployDirectory $name) -Force
    }
    Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $DeployDirectory 'build-manifest.json') -Force

    # Deploy core runtime outputs explicitly for both configurations. Debug
    # projects retain historical post-build copy events, but correctness must
    # not depend on those project-local side effects.
    foreach ($name in $requiredBinaries + $optionalSymbols) {
        $source = Join-Path $outputDirectory $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            Copy-Item -LiteralPath $source -Destination (Join-Path $DeployDirectory $name) -Force
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
    foreach ($name in $legalNotices) {
        $source = Join-Path $repoRoot $name
        $deployed = Join-Path $DeployDirectory $name
        if ((Get-Sha256 $source) -ne (Get-Sha256 $deployed)) {
            throw "Deployed legal notice does not match the repository copy: $name"
        }
    }
}

Write-Host ''
Write-Host "Successful $Configuration build:"
Get-Item ($requiredBinaries | ForEach-Object { Join-Path $outputDirectory $_ }) |
    Select-Object Name, Length, LastWriteTime |
    Format-Table -AutoSize
Write-Host "Engine modules: $($engineBinaries.Count) rebuilt and verified"
Write-Host "Campaign modules: $($campaignBinaries.Count) built and verified"
Write-Host "Build manifest: $manifestPath"
if (-not $NoDeploy) {
    Write-Host "Campaign deployment: $(Join-Path $DeployDirectory 'Scripts')"
}
