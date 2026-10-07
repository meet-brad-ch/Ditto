# Complexity gate: measures cyclomatic complexity (lizard) of the project's own C/C++ sources.
# No function of Ditto's own code (lib\, tests\ and the legacy app alike) may reach CC 10.
# Phase L2 took the legacy functions below the limit, so the former baseline ratchet is gone.
# Untouched third-party code (tools\thirdparty.txt) is not measured (tools\gates\sources.ps1).
param(
    [Parameter(Mandatory)] [string] $Repo
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
$limit = 10

. (Join-Path $PSScriptRoot 'sources.ps1')
$files = Get-OwnSources $Repo
$outDir = Join-Path $Repo 'build\logs'
New-Item -ItemType Directory $outDir -Force | Out-Null
$fileList = Join-Path $outDir 'complexity-files.txt'
[IO.File]::WriteAllLines($fileList, [string[]]$files)

Push-Location $Repo
try { $csv = & lizard -l cpp --csv -f $fileList 2>&1; $code = $LASTEXITCODE }
finally { Pop-Location }
if ($code -ne 0) { $csv | Select-Object -First 10 | ForEach-Object { Say "complexity: $_" }; Say "complexity: FAILED lizard exit $code"; exit 1 }

$header = 'nloc', 'ccn', 'tokens', 'params', 'length', 'location', 'file', 'function', 'longName', 'start', 'end'
$functions = @($csv | ConvertFrom-Csv -Header $header)
if ($functions.Count -eq 0) { Say 'complexity: FAILED lizard reported no functions'; exit 1 }

$findings = 0
foreach ($f in $functions) {
    $ccn = [int]$f.ccn
    if ($ccn -lt $limit) { continue }
    Say ("complexity: FAIL {0}:{1} {2} has CC {3} (limit {4})" -f $f.file, $f.start, $f.function, $ccn, ($limit - 1))
    $findings++
}
$maxCcn = ($functions | ForEach-Object { [int]$_.ccn } | Measure-Object -Maximum).Maximum
Say ("complexity: {0} functions in {1} files; max CC {2}" -f $functions.Count, $files.Count, $maxCcn)
if ($findings -gt 0) { Say "complexity: FAILED $findings functions at CC >= $limit"; exit 1 }
Say "complexity: ok   every function below CC $limit"
exit 0
