# Allocation gate: no raw new/delete/malloc/calloc/realloc/free in Ditto's own code (owner rule:
# smart pointers and containers only).
#  - contract code (lib\, tests\): none, ever;
#  - legacy code: the count per file is held in tools\baselines\allocation.tsv by tools\ratchet.ps1,
#    so it may only shrink (Phase L1 takes it to zero).
# Not allocations: deleted functions ('= delete'), comments, the text of string and character
# literals, #include lines ('#include <new>'), MFC's '#define new DEBUG_NEW'. Allowed: a line that hands ownership to a framework
# owner and says so in a comment, '// ownership: <who owns it>' (a self-deleting MFC window, a COM
# object released by its last Release()).
# Untouched third-party code (tools\thirdparty.txt) is skipped.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
. (Join-Path $PSScriptRoot 'sources.ps1')

$allocPattern = '\bnew\b|\bdelete\b|\b(malloc|calloc|realloc|free)\s*\('

# The code part of each line: comments, string and character literals blanked
function Get-CodeLines([string] $path) {
    $inBlock = $false   # inside a /* ... */ (or Doxygen /** ... */) comment
    foreach ($line in [IO.File]::ReadLines($path)) {
        $code = ''
        $rest = $line
        while ($rest.Length -gt 0) {
            if ($inBlock) {
                $end = $rest.IndexOf('*/')
                if ($end -lt 0) { $rest = '' } else { $rest = $rest.Substring($end + 2); $inBlock = $false }
                continue
            }
            $lineComment = $rest.IndexOf('//')
            $blockStart = $rest.IndexOf('/*')
            if ($blockStart -ge 0 -and ($lineComment -lt 0 -or $blockStart -lt $lineComment)) {
                $code += $rest.Substring(0, $blockStart); $rest = $rest.Substring($blockStart + 2); $inBlock = $true
            }
            elseif ($lineComment -ge 0) { $code += $rest.Substring(0, $lineComment); $rest = '' }
            else { $code += $rest; $rest = '' }
        }
        $code = $code -replace '=\s*delete\b', ''
        $code = $code -replace '"(\\.|[^"\\])*"', '""' -replace "'(\\.|[^'\\])*'", "''"
        [pscustomobject]@{ Text = $line; Code = $code }
    }
}

$files = Get-OwnSources $Repo
$contractFindings = 0
$legacyCounts = [ordered]@{}
foreach ($rel in $files) {
    $isContract = $rel -match '^(lib|tests)\\'
    $n = 0
    $count = 0
    foreach ($line in Get-CodeLines (Join-Path $Repo $rel)) {
        $n++
        if ($line.Code -match '^\s*#\s*define\s+new\s+DEBUG_NEW\b') { continue }
        if ($line.Code -match '^\s*#\s*include\b') { continue }   # e.g. #include <new>: a header, not an allocation
        if ($line.Text -match '//\s*ownership:') { continue }
        if ($line.Code -cnotmatch $allocPattern) { continue }
        if ($isContract) {
            Say ("allocation: FAIL contract code {0}:{1}: {2}" -f $rel, $n, $line.Text.Trim())
            $contractFindings++
        }
        else { $count++ }
    }
    if (-not $isContract -and $count -gt 0) { $legacyCounts[$rel.ToLowerInvariant()] = $count }
}

$total = 0; foreach ($v in $legacyCounts.Values) { $total += $v }
Say ("allocation: {0} files; contract code {1}; legacy code {2} raw allocations in {3} files" -f $files.Count, $(if ($contractFindings) { "$contractFindings FAILED" } else { 'none' }), $total, $legacyCounts.Count)
if ($contractFindings -gt 0) { Say "allocation: FAILED $contractFindings raw allocations in lib\ or tests\"; exit 1 }

$outDir = Join-Path $Repo 'build\logs'
New-Item -ItemType Directory $outDir -Force | Out-Null
$current = Join-Path $outDir 'allocation.tsv'
$lines = @($legacyCounts.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($legacyCounts[$_])" })
[IO.File]::WriteAllLines($current, [string[]]$lines, [Text.UTF8Encoding]::new($false))
$ratchetArgs = @{ Name = 'allocation'; Current = $current; Baseline = (Join-Path $Repo 'tools\baselines\allocation.tsv') }
if ($Update) { $ratchetArgs['Update'] = $true }
& (Join-Path $Repo 'tools\ratchet.ps1') @ratchetArgs
exit $LASTEXITCODE
