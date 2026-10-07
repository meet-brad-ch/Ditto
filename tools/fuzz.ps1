# Fuzzes the lib\DittoCore parsers with libFuzzer (tests\fuzz\DittoFuzz.vcxproj, built by the
# Release|x64 solution build). For each target: writes its generated seed corpus into
# build\fuzz\<target>\corpus (the corpus grows between runs), then fuzzes for -Seconds.
# A crash, sanitizer report, leak or timeout fails the run; libFuzzer saves the input that
# triggered it in build\fuzz\<target>\ (crash-*, leak-*, timeout-*). Every finding must become a
# unit test in tests\ before it is fixed.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\fuzz.ps1 [-Target <name>] [-Seconds 60]
param([string] $Target = '', [int] $Seconds = 60)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

$repo = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repo 'build\DittoFuzz\x64\Release\DittoFuzz.exe'
if (-not (Test-Path $exe)) { Say "fuzz: FAILED $exe not found (build Release|x64 first)"; exit 1 }
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
$dumpbin = Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') | Sort-Object FullName | Select-Object -Last 1
$env:PATH = "$($dumpbin.DirectoryName);$env:PATH"   # clang_rt.asan_dynamic-x86_64.dll lives next to dumpbin
$ErrorActionPreference = 'Continue'                # libFuzzer and ASan write to stderr

# the binary lists its targets when none is selected
Remove-Item Env:DITTO_FUZZ_TARGET -ErrorAction SilentlyContinue
$listing = (& $exe 2>&1 | ForEach-Object { "$_" }) -join ' '
if ($listing -notmatch 'must name a target:\s*(?<names>.+)$') { Say "fuzz: FAILED could not list targets: $listing"; exit 1 }
$targets = @($Matches['names'].Trim() -split '\s+')
if ($Target) {
    if ($targets -notcontains $Target) { Say "fuzz: FAILED unknown target '$Target' (targets: $($targets -join ', '))"; exit 1 }
    $targets = @($Target)
}

$failed = 0
foreach ($t in $targets) {
    $dir = Join-Path $repo "build\fuzz\$t"
    $corpus = Join-Path $dir 'corpus'
    $env:DITTO_FUZZ_TARGET = $t
    $env:DITTO_FUZZ_WRITE_SEEDS = $corpus
    & $exe 2>&1 | Out-Null
    Remove-Item Env:DITTO_FUZZ_WRITE_SEEDS
    if ($LASTEXITCODE -ne 0) { Say "fuzz: FAILED $t could not write its seeds"; $failed++; continue }

    Get-ChildItem (Join-Path $dir '*') -File -Include crash-*, leak-*, timeout-*, oom-* -ErrorAction SilentlyContinue | Remove-Item
    Say "fuzz: start $t for $Seconds s"
    # libFuzzer's INITED and pulse lines (at doubling run counts) are echoed as progress
    $out = @(& $exe $corpus "-max_total_time=$Seconds" '-timeout=10' "-artifact_prefix=$dir\" 2>&1 | ForEach-Object {
        $line = "$_"
        if ($line -match '^#\d+\s+(INITED|pulse)') { Say "fuzz: $t $line" | Out-Host }
        $line
    })
    $code = $LASTEXITCODE
    $runs = ($out | Where-Object { $_ -match '^Done (\d+) runs' } | Select-Object -Last 1) -replace '^Done (\d+) runs.*$', '$1'
    $findings = @(Get-ChildItem (Join-Path $dir '*') -File -Include crash-*, leak-*, timeout-*, oom-* -ErrorAction SilentlyContinue)
    if ($code -ne 0 -or $findings.Count -gt 0) {
        $out | Where-Object { $_ -match 'ERROR: |SUMMARY|#\d+ 0x|deadly signal|Test unit written' } | Select-Object -First 12 | ForEach-Object { Say "    $_" }
        $findings | ForEach-Object { Say "fuzz: FAIL $t finding saved: $($_.FullName.Substring($repo.Length + 1))" }
        Say "fuzz: FAILED $t (exit $code)"
        $failed++
    }
    else {
        $size = @(Get-ChildItem $corpus -File).Count
        Say "fuzz: ok   $t $runs runs in $Seconds s, corpus $size inputs"
    }
}
Remove-Item Env:DITTO_FUZZ_TARGET -ErrorAction SilentlyContinue
if ($failed -gt 0) { Say "fuzz: FAILED $failed of $($targets.Count) targets"; exit 1 }
Say "fuzz: ok   $($targets.Count) targets, $Seconds s each, no findings"
exit 0
