# Warnings gate: counts the compiler and linker warnings of a full build log and checks them
# against tools\baselines\warnings.tsv with tools\ratchet.ps1 (counts may only shrink).
# A warning is identified by (location, line, code) so the same header warning reported by many
# translation units counts once. Counts are kept per (code, location).
# Locations are repo-relative and lower-case; outside the repo only the file name is kept
# (SDK and toolset paths differ between machines); command-line and linker warnings without a
# file are counted under their project.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [Parameter(Mandatory)] [string] $Log,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

function Get-Location([string] $path) {
    $full = $path.Trim()
    if ($full.StartsWith($Repo, [StringComparison]::OrdinalIgnoreCase)) { return $full.Substring($Repo.Length).TrimStart('\').ToLowerInvariant() }
    return ('external:' + [IO.Path]::GetFileName($full)).ToLowerInvariant()
}

if (-not (Test-Path $Log)) { Say "warnings: FAILED build log $Log not found"; exit 1 }
$pattern = '^\s*(?:\d+>)?(?<loc>.+?)\s*:\s+(?:command line\s+)?warning\s+(?<code>[A-Z]+\d+)\s*:.*?(?:\[(?<proj>[^\]]+\.vcxproj)\])?\s*$'
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$counts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
foreach ($line in [IO.File]::ReadLines($Log)) {
    if ($line -notmatch $pattern) { continue }
    $code = $Matches['code']
    $loc = $Matches['loc']
    $lineNo = '0'
    if ($loc -match '^(?<file>.+?)\((?<line>\d+)(?:,\d+)?\)$') { $loc = $Matches['file']; $lineNo = $Matches['line'] }
    if (-not [IO.Path]::IsPathRooted($loc) -or $loc -match '\.obj$|\.lib$') {
        $proj = if ($line -match '\[(?<p>[^\]]+\.vcxproj)\]') { $Matches['p'] } else { 'unknown-project' }
        $loc = $proj
    }
    $where = Get-Location $loc
    if (-not $seen.Add("$where|$lineNo|$code")) { continue }
    $key = "$code`t$where"
    if ($counts.ContainsKey($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
}

$current = Join-Path (Split-Path -Parent $Log) 'warnings.tsv'
$lines = @($counts.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($counts[$_])" })
[IO.File]::WriteAllLines($current, [string[]]$lines, [Text.UTF8Encoding]::new($false))
$total = 0; foreach ($v in $counts.Values) { $total += $v }
Say "warnings: $total distinct warnings in $($lines.Count) (code, file) pairs, measured from $Log"

$ratchetArgs = @{ Name = 'warnings'; Current = $current; Baseline = (Join-Path $Repo 'tools\baselines\warnings.tsv') }
if ($Update) { $ratchetArgs['Update'] = $true }
& (Join-Path $Repo 'tools\ratchet.ps1') @ratchetArgs
exit $LASTEXITCODE
