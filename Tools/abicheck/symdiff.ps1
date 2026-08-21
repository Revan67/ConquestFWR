$ErrorActionPreference = 'Stop'
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"

$retailDll = "D:\Dev\ConquestFWR\Conquest Frontier Wars\mission.dll"
$oursDll   = "D:\Games\GOG\Conquest Frontier Wars\Mission.dll"
$scriptDir = "D:\Games\GOG\Conquest Frontier Wars\Scripts"

function Get-Exports($path) {
    $out = & $dumpbin /exports $path 2>$null
    $names = New-Object System.Collections.Generic.HashSet[string]
    $started = $false
    foreach ($l in $out) {
        if ($l -match '^\s+ordinal\s+hint\s+RVA\s+name') { $started = $true; continue }
        if (-not $started) { continue }
        # "      1    0 00001000 ?Name@@YAXXZ"  (forwarders/data have variants)
        if ($l -match '^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(\S+)') { [void]$names.Add($Matches[1]) }
        elseif ($l -match '^\s*\d+\s+[0-9A-Fa-f]+\s+(\S+)$')             { [void]$names.Add($Matches[1]) }
    }
    return $names
}

# Imports that a script DLL pulls specifically from mission.dll / conquest.exe
function Get-ImportsFrom($path, $modulePattern) {
    $out = & $dumpbin /imports $path 2>$null
    $names = New-Object System.Collections.Generic.HashSet[string]
    $inTarget = $false
    foreach ($l in $out) {
        if ($l -match '^\s{2,}(\S+\.(dll|exe))\s*$') {
            $inTarget = ($Matches[1] -match $modulePattern)
            continue
        }
        if (-not $inTarget) { continue }
        # "          1B4  ?Name@@..."   or  ordinal-only entries
        if ($l -match '^\s+[0-9A-Fa-f]+\s+(\S+)\s*$') { [void]$names.Add($Matches[1]) }
    }
    return $names
}

$retail = Get-Exports $retailDll
$ours   = Get-Exports $oursDll

Write-Output "=== EXPORT COUNTS ==="
Write-Output ("retail mission.dll : {0}" -f $retail.Count)
Write-Output ("our    Mission.dll : {0}" -f $ours.Count)

$missing = @($retail | Where-Object { -not $ours.Contains($_) })
$added   = @($ours   | Where-Object { -not $retail.Contains($_) })

Write-Output ""
Write-Output ("=== IN RETAIL BUT MISSING FROM OURS: {0} ===" -f $missing.Count)
$missing | Sort-Object | Select-Object -First 60 | ForEach-Object { Write-Output "  $_" }

Write-Output ""
Write-Output ("=== IN OURS BUT NOT RETAIL (additions): {0} ===" -f $added.Count)
$added | Sort-Object | Select-Object -First 60 | ForEach-Object { Write-Output "  $_" }

# --- what the retail scripts actually demand ---
Write-Output ""
Write-Output "=== SCRIPT IMPORT SATISFACTION ==="
$allUnsat = @{}
foreach ($f in Get-ChildItem $scriptDir -Filter *.dll | Sort-Object Name) {
    $need = Get-ImportsFrom $f.FullName 'mission|conquest'
    if ($need.Count -eq 0) { Write-Output ("{0,-16} (no mission/conquest imports parsed)" -f $f.Name); continue }
    $unsat = @($need | Where-Object { -not $ours.Contains($_) })
    Write-Output ("{0,-16} needs {1,4}   UNSATISFIED {2}" -f $f.Name, $need.Count, $unsat.Count)
    foreach ($u in $unsat) { $allUnsat[$u] = $true }
}

Write-Output ""
Write-Output ("=== DISTINCT UNSATISFIED SYMBOLS: {0} ===" -f $allUnsat.Keys.Count)
$allUnsat.Keys | Sort-Object | Select-Object -First 80 | ForEach-Object { Write-Output "  $_" }
