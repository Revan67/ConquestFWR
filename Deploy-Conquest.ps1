[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Final')]
    [string]$Configuration,

    [string]$DeployDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($DeployDirectory)) {
    $DeployDirectory = if ($Configuration -eq 'Debug') {
        'D:\Games\GOG\Conquest Frontier Wars'
    }
    else {
        'D:\Games\GOG-Final\Conquest Frontier Wars'
    }
}

function Get-Sha256([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try {
        $sha256 = [Security.Cryptography.SHA256]::Create()
        try {
            return ([BitConverter]::ToString($sha256.ComputeHash($stream))).Replace('-', '')
        }
        finally { $sha256.Dispose() }
    }
    finally { $stream.Dispose() }
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
        }
    $orderedPaths = [System.Collections.Generic.SortedSet[string]]::new([StringComparer]::Ordinal)
    foreach ($path in $paths) { [void]$orderedPaths.Add($path) }

    $sha256 = [Security.Cryptography.SHA256]::Create()
    try {
        $builder = New-Object Text.StringBuilder
        foreach ($relativePath in $orderedPaths) {
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
    finally { $sha256.Dispose() }
}

if (Get-Process -Name Conquest -ErrorAction SilentlyContinue) {
    throw 'Conquest is running. Exit the game before deploying a build.'
}
if (-not (Test-Path -LiteralPath $DeployDirectory -PathType Container)) {
    throw "Deployment directory does not exist: $DeployDirectory"
}

$manifestPath = Join-Path $repoRoot "Build-Manifests\$Configuration-latest.json"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Build manifest does not exist: $manifestPath"
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -lt 2 -or [string]::IsNullOrWhiteSpace($manifest.buildInputFingerprint)) {
    throw 'This manifest predates build-input verification. Run a full rebuild first.'
}
if ($manifest.configuration -ne $Configuration) {
    throw "Manifest configuration is $($manifest.configuration), not $Configuration."
}
$sourceRevision = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($sourceRevision -ne $manifest.sourceRevision) {
    throw 'HEAD no longer matches the revision recorded in the build manifest.'
}
$inputFingerprint = Get-BuildInputFingerprint
if ($inputFingerprint -ne $manifest.buildInputFingerprint) {
    throw 'Build inputs changed after the recorded full rebuild. Rebuild before deploying.'
}

foreach ($artifact in $manifest.artifacts) {
    $source = Join-Path $repoRoot ([string]$artifact.path)
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Manifest artifact is missing: $source"
    }
    if ((Get-Sha256 $source) -ne $artifact.sha256) {
        throw "Manifest artifact hash mismatch: $source"
    }
}

$copied = New-Object System.Collections.Generic.List[object]
foreach ($artifact in $manifest.artifacts) {
    $relative = ([string]$artifact.path).Replace('/', '\')
    $destination = $null
    if ($relative -match '^Code\\App\\Src\\(Debug|Final)\\([^\\]+\.(dll|exe|pdb))$') {
        $destination = Join-Path $DeployDirectory $Matches[2]
    }
    elseif ($relative -match '^Code\\Libs\\ImplicitDLL\\(debug|release)\\DACOM\.dll$') {
        $destination = Join-Path $DeployDirectory 'DACOM.dll'
    }
    elseif ($relative -match '^Code\\Libs\\ExplicitDLL\\(debug|release)\\([^\\]+\.dll)$') {
        $destination = Join-Path (Join-Path $DeployDirectory 'DLL') $Matches[2]
    }
    elseif ($relative -match '^Code\\App\\Src\\Scripts\\Build\\(Debug|Final)\\([^\\]+\.dll)$') {
        $destination = Join-Path (Join-Path $DeployDirectory 'Scripts') $Matches[2]
    }

    if ($destination) {
        $destinationParent = Split-Path -Parent $destination
        if (-not (Test-Path -LiteralPath $destinationParent -PathType Container)) {
            New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
        }
        $source = Join-Path $repoRoot $relative
        Copy-Item -LiteralPath $source -Destination $destination -Force
        $copied.Add([pscustomobject]@{ Source=$source; Destination=$destination; Sha256=$artifact.sha256 })
    }
}

foreach ($item in $copied) {
    if ((Get-Sha256 $item.Destination) -ne $item.Sha256) {
        throw "Deployed artifact hash mismatch: $($item.Destination)"
    }
}

foreach ($name in @('Conquest Source License.txt', 'LEGAL.md', 'MODIFICATIONS.md')) {
    $source = Join-Path $repoRoot $name
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Required legal notice is missing: $source"
    }
    $destination = Join-Path $DeployDirectory $name
    Copy-Item -LiteralPath $source -Destination $destination -Force
    if ((Get-Sha256 $source) -ne (Get-Sha256 $destination)) {
        throw "Deployed legal notice hash mismatch: $name"
    }
}

$launchers = if ($Configuration -eq 'Debug') { @('run-debug.cmd', 'run-debug-capture.cmd') } else { @('run-final.cmd') }
foreach ($name in $launchers) {
    $source = Join-Path $repoRoot $name
    $destination = Join-Path $DeployDirectory $name
    Copy-Item -LiteralPath $source -Destination $destination -Force
    if ((Get-Sha256 $source) -ne (Get-Sha256 $destination)) {
        throw "Deployed launcher hash mismatch: $name"
    }
}

Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $DeployDirectory 'build-manifest.json') -Force
Write-Host "Deployed and hash-verified $($copied.Count) $Configuration artifacts to $DeployDirectory"
