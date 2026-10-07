# Ratchet: compares measured counts with a committed baseline that may only shrink.
# Both files are TSV lines "key<TAB>count" (the key may itself contain tabs; the count is the last
# field). A key whose count rises above the baseline, or a key missing from the baseline, fails.
# With -Update the baseline is rewritten from the measurement, but only when nothing rose: a
# baseline never grows. A missing baseline is created by -Update.
# Exit code: 0 ok, 1 failed. Prints timestamped lines prefixed with -Name.
param(
    [Parameter(Mandatory)] [string] $Name,
    [Parameter(Mandatory)] [string] $Current,
    [Parameter(Mandatory)] [string] $Baseline,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

function Read-Counts([string] $path) {
    $counts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        if (-not $line.Trim()) { continue }
        $tab = $line.LastIndexOf("`t")
        if ($tab -lt 0) { throw "${path}: line without a tab: '$line'" }
        $counts[$line.Substring(0, $tab)] = [int]$line.Substring($tab + 1)
    }
    return , $counts
}
function Get-Total($counts) { $sum = 0; foreach ($v in $counts.Values) { $sum += $v }; return $sum }

$measured = Read-Counts $Current
$haveBaseline = Test-Path $Baseline
if (-not $haveBaseline -and -not $Update) {
    Say "${Name}: FAILED no baseline at $Baseline (create it with -UpdateBaselines)"
    exit 1
}
if (-not $haveBaseline) {
    $lines = @($measured.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($measured[$_])" })
    [IO.File]::WriteAllLines($Baseline, [string[]]$lines, [Text.UTF8Encoding]::new($false))
    Say ("{0}: baseline created, {1} entries, total {2}" -f $Name, $lines.Count, (Get-Total $measured))
    exit 0
}
$base = Read-Counts $Baseline

$rises = [Collections.Generic.List[string]]::new()
foreach ($key in $measured.Keys) {
    $was = if ($base.ContainsKey($key)) { $base[$key] } else { 0 }
    if ($measured[$key] -gt $was) { $rises.Add(("{0}  (baseline {1}, now {2})" -f ($key -replace "`t", '  '), $was, $measured[$key])) }
}
$shrunk = @($base.Keys | Where-Object { -not $measured.ContainsKey($_) -or $measured[$_] -lt $base[$_] }).Count

if ($rises.Count -gt 0) {
    $rises | Sort-Object | Select-Object -First 40 | ForEach-Object { Say "${Name}: FAIL $_" }
    if ($rises.Count -gt 40) { Say "${Name}: ... and $($rises.Count - 40) more" }
    Say ("{0}: FAILED {1} entries above the baseline (total now {2}, baseline {3})" -f $Name, $rises.Count, (Get-Total $measured), (Get-Total $base))
    exit 1
}

if ($Update) {
    $lines = @($measured.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($measured[$_])" })
    [IO.File]::WriteAllLines($Baseline, [string[]]$lines, [Text.UTF8Encoding]::new($false))
    Say ("{0}: baseline written, {1} entries, total {2}" -f $Name, $lines.Count, (Get-Total $measured))
    exit 0
}
$note = if ($shrunk -gt 0) { "; $shrunk entries below the baseline, shrink it with -UpdateBaselines" } else { '' }
Say ("{0}: ok   total {1} (baseline {2}){3}" -f $Name, (Get-Total $measured), (Get-Total $base), $note)
exit 0
