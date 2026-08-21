$ErrorActionPreference = 'Stop'
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
$game = "D:\Games\GOG\Conquest Frontier Wars"

# Every module every retail script imports from, with counts
$modCount = @{}
$perScript = @{}
foreach ($f in Get-ChildItem "$game\Scripts" -Filter *.dll) {
    $out = & $dumpbin /imports $f.FullName 2>$null
    $cur = $null
    foreach ($l in $out) {
        if ($l -match '^\s{2,}(\S+\.(dll|exe))\s*$') {
            $cur = $Matches[1]
            if (-not $modCount.ContainsKey($cur)) { $modCount[$cur] = 0 }
            $modCount[$cur]++
            continue
        }
        if ($cur -and $l -match '^\s+[0-9A-Fa-f]+\s+(\S+)\s*$') {
            if (-not $perScript.ContainsKey($cur)) { $perScript[$cur] = New-Object System.Collections.Generic.HashSet[string] }
            [void]$perScript[$cur].Add($Matches[1])
        }
    }
}

Write-Output "=== MODULES THE RETAIL SCRIPTS BIND TO ==="
foreach ($k in ($modCount.Keys | Sort-Object)) {
    $syms = 0; if ($perScript.ContainsKey($k)) { $syms = $perScript[$k].Count }
    Write-Output ("  {0,-20} referenced by {1,2} scripts, {2,4} distinct symbols" -f $k, $modCount[$k], $syms)
}

# For each module WE build, check satisfaction against our freshly built copy
function Get-Exports($path) {
    $out = & $dumpbin /exports $path 2>$null
    $names = New-Object System.Collections.Generic.HashSet[string]
    $started = $false
    foreach ($l in $out) {
        if ($l -match '^\s+ordinal\s+hint\s+RVA\s+name') { $started = $true; continue }
        if (-not $started) { continue }
        if ($l -match '^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(\S+)') { [void]$names.Add($Matches[1]) }
        elseif ($l -match '^\s*\d+\s+[0-9A-Fa-f]+\s+(\S+)$')             { [void]$names.Add($Matches[1]) }
    }
    return $names
}

$ourBuilt = @{
    'Mission.dll'  = "$game\Mission.dll"
    'Globals.dll'  = "$game\Globals.dll"
    'Trim.dll'     = "$game\Trim.dll"
    'ZBatcher.dll' = "$game\ZBatcher.dll"
    'Conquest.exe' = "$game\Conquest.exe"
}

Write-Output ""
Write-Output "=== SATISFACTION AGAINST OUR FRESH BUILDS ==="
foreach ($k in ($perScript.Keys | Sort-Object)) {
    $match = $ourBuilt.Keys | Where-Object { $_ -ieq $k }
    if (-not $match) { Write-Output ("  {0,-20} (retail/3rd-party, not built by us - skipped)" -f $k); continue }
    $ex = Get-Exports $ourBuilt[$match]
    $unsat = @($perScript[$k] | Where-Object { -not $ex.Contains($_) })
    Write-Output ("  {0,-20} demanded {1,4}   UNSATISFIED {2}" -f $k, $perScript[$k].Count, $unsat.Count)
    $unsat | Sort-Object | ForEach-Object { Write-Output "      MISSING: $_" }
}
