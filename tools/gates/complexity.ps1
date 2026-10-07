# Complexity gate: measures cyclomatic complexity (lizard) of the project's own C/C++ sources.
#  - contract code (lib\, tests\): no function may reach CC 10, ever;
#  - legacy code: every function at CC >= 10 is held in tools\baselines\complexity.tsv by
#    tools\ratchet.ps1, so no new function may reach 10 and no listed function may get worse.
# Vendored code (src\sqlite, src\QRCode, src\TinyXml) is not measured.
# Functions are keyed by (file, qualified name); overloads share the key and keep the highest CC.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
$limit = 10

$dirs = 'src', 'Shared', 'Addins', 'ICU_Loader', 'focusdll', 'lib', 'tests'
$vendored = '^src\\(sqlite|QRCode|TinyXml)\\'
$files = foreach ($d in $dirs) {
    Get-ChildItem (Join-Path $Repo $d) -Recurse -File -Include *.c, *.cpp, *.h, *.hpp |
        ForEach-Object { $_.FullName.Substring($Repo.Length + 1) } |
        Where-Object { $_ -notmatch $vendored }
}
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

$worst = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
$contractFindings = 0
foreach ($f in $functions) {
    $ccn = [int]$f.ccn
    if ($ccn -lt $limit) { continue }
    $file = $f.file.ToLowerInvariant()
    if ($file -match '^(lib|tests)\\') {
        Say ("complexity: FAIL contract code {0} {1} has CC {2} (limit {3})" -f $f.file, $f.function, $ccn, ($limit - 1))
        $contractFindings++
        continue
    }
    $key = "$file`t$($f.function)"
    if (-not $worst.ContainsKey($key) -or $worst[$key] -lt $ccn) { $worst[$key] = $ccn }
}
$contractCcn = @($functions | Where-Object { $_.file -match '^(lib|tests)\\' } | ForEach-Object { [int]$_.ccn })
if ($contractCcn.Count -eq 0) { Say 'complexity: FAILED no contract-code functions measured in lib\ and tests\'; exit 1 }
$maxContract = ($contractCcn | Measure-Object -Maximum).Maximum
Say ("complexity: {0} functions in {1} files; contract code max CC {2}; {3} legacy functions at CC >= {4}" -f $functions.Count, $files.Count, $maxContract, $worst.Count, $limit)
if ($contractFindings -gt 0) { Say "complexity: FAILED $contractFindings contract-code functions at CC >= $limit"; exit 1 }

$current = Join-Path $outDir 'complexity.tsv'
$lines = @($worst.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($worst[$_])" })
[IO.File]::WriteAllLines($current, [string[]]$lines, [Text.UTF8Encoding]::new($false))
$ratchetArgs = @{ Name = 'complexity'; Current = $current; Baseline = (Join-Path $Repo 'tools\baselines\complexity.tsv') }
if ($Update) { $ratchetArgs['Update'] = $true }
& (Join-Path $Repo 'tools\ratchet.ps1') @ratchetArgs
exit $LASTEXITCODE
