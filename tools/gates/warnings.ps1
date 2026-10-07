# Warnings gate: counts the warnings of a full build log and checks them against a baseline with
# tools\ratchet.ps1 (counts may only shrink).
#   -Kind build    compiler and linker warnings  -> tools\baselines\warnings.tsv
#   -Kind analyze  code analysis (/analyze) warnings C6xxx, C26xxx, C28xxx, C33xxx
#                                                -> tools\baselines\analyze.tsv
# A warning is identified by (location, line, code) so the same header warning reported by many
# translation units counts once. Counts are kept per (code, location).
# Locations are repo-relative and lower-case; outside the repo only the file name is kept
# (SDK and toolset paths differ between machines); command-line and linker warnings without a
# file are counted under their project.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [Parameter(Mandatory)] [string] $Log,
    [ValidateSet('build', 'analyze')] [string] $Kind = 'build',
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

$analysisCode = '^C(6\d{3}|2[68]\d{3}|33\d{3})$'
$name = if ($Kind -eq 'build') { 'warnings' } else { 'analyze' }
$baseline = Join-Path $Repo "tools\baselines\$name.tsv"

function Get-Location([string] $path) {
    $full = $path.Trim()
    if ($full.StartsWith($Repo, [StringComparison]::OrdinalIgnoreCase)) { return $full.Substring($Repo.Length).TrimStart('\').ToLowerInvariant() }
    return ('external:' + [IO.Path]::GetFileName($full)).ToLowerInvariant()
}

if (-not (Test-Path $Log)) { Say "${name}: FAILED build log $Log not found"; exit 1 }
# MSBuild prefixes lines with the node number under /m ("6>")
$pattern = '^\s*(?:\d+>)?(?<loc>.+?)\s*:\s+(?:command line\s+)?warning\s+(?<code>[A-Z]+\d+)\s*:'
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$counts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
foreach ($line in [IO.File]::ReadLines($Log)) {
    if ($line -notmatch $pattern) { continue }
    $code = $Matches['code']
    $loc = $Matches['loc']
    if ([regex]::IsMatch($code, $analysisCode) -ne ($Kind -eq 'analyze')) { continue }   # IsMatch: keeps $Matches
    $lineNo = '0'
    if ($loc -match '^(?<file>.+?)\((?<line>\d+)(?:,\d+)?\)$') { $loc = $Matches['file']; $lineNo = $Matches['line'] }
    if (-not [IO.Path]::IsPathRooted($loc) -or $loc -match '\.obj$|\.lib$') {
        $loc = if ($line -match '\[(?<p>[^\]]+\.vcxproj)\]') { $Matches['p'] } else { 'unknown-project' }
    }
    $where = Get-Location $loc
    if (-not $seen.Add("$where|$lineNo|$code")) { continue }
    $key = "$code`t$where"
    if ($counts.ContainsKey($key)) { $counts[$key]++ } else { $counts[$key] = 1 }
}

$current = Join-Path (Split-Path -Parent $Log) "$name.tsv"
$lines = @($counts.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($counts[$_])" })
[IO.File]::WriteAllLines($current, [string[]]$lines, [Text.UTF8Encoding]::new($false))
$total = 0; foreach ($v in $counts.Values) { $total += $v }
Say "${name}: $total distinct warnings in $($lines.Count) (code, file) pairs, measured from $Log"

$ratchetArgs = @{ Name = $name; Current = $current; Baseline = $baseline }
if ($Update) { $ratchetArgs['Update'] = $true }
& (Join-Path $Repo 'tools\ratchet.ps1') @ratchetArgs
exit $LASTEXITCODE
