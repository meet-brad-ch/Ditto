# Coverage gate: line coverage of the contract library lib\DittoCore, measured with Microsoft code
# coverage (ships with Visual Studio, dynamic native instrumentation) over the Debug|x64 test build
# (the Release test build uses AddressSanitizer, which the instrumentation does not combine with).
# Fails below the owner's target of 90 % line coverage. Lists every uncovered line.
# Branch coverage is not measured by this tool: it is reported as NOT VERIFIED, never assumed.
# Writes build\coverage\DittoCore.cobertura.xml.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [Parameter(Mandatory)] [string] $VsPath,
    [double] $MinimumLineRate = 0.90
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

$console = Join-Path $VsPath 'Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe'
if (-not (Test-Path $console)) { Say "coverage: FAILED Microsoft.CodeCoverage.Console.exe not found under $VsPath"; exit 1 }

$msbuild = Join-Path $VsPath 'MSBuild\Current\Bin\amd64\MSBuild.exe'
$out = & $msbuild (Join-Path $Repo 'tests\DittoTests.vcxproj') /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$Repo\" /m /nologo /v:m 2>&1
if ($LASTEXITCODE -ne 0) {
    $out | Where-Object { "$_" -match 'error' } | Select-Object -First 20 | ForEach-Object { Say "  $_" }
    Say "coverage: FAILED Debug|x64 test build exit $LASTEXITCODE"; exit 1
}
$testExe = Join-Path $Repo 'build\DittoTests\x64\Debug\DittoTests.exe'

$outDir = Join-Path $Repo 'build\coverage'
New-Item -ItemType Directory $outDir -Force | Out-Null
$report = Join-Path $outDir 'DittoCore.cobertura.xml'
$settings = Join-Path $PSScriptRoot 'coverage.config.xml'
$run = & $console collect $testExe --gtest_brief=1 --settings $settings --output $report --output-format cobertura --nologo 2>&1
if ($LASTEXITCODE -ne 0) {
    $run | Select-Object -Last 20 | ForEach-Object { Say "  $_" }
    Say "coverage: FAILED the instrumented test run exited $LASTEXITCODE"; exit 1
}

[xml]$xml = [IO.File]::ReadAllText($report)
$classes = @($xml.coverage.packages.package.classes.class | Where-Object { $_.filename -match '\\lib\\DittoCore\\' })
if ($classes.Count -eq 0) { Say "coverage: FAILED no lib\DittoCore source in $report"; exit 1 }
$valid = 0
$covered = 0
$uncovered = [Collections.Generic.List[string]]::new()
foreach ($class in $classes) {
    $file = $class.filename.Substring($Repo.Length + 1)
    foreach ($line in @($class.lines.line)) {
        $valid++
        if ([int]$line.hits -gt 0) { $covered++ } else { $uncovered.Add("${file}:$($line.number)") }
    }
}
$rate = $covered / $valid
$uncovered | Sort-Object -Unique | ForEach-Object { Say "coverage: uncovered $_" }
Say ("coverage: line {0:P1} of lib\DittoCore ({1}/{2} lines, {3} files), minimum {4:P0}" -f $rate, $covered, $valid, $classes.Count, $MinimumLineRate)
Say 'coverage: branch NOT VERIFIED (Microsoft code coverage reports no branch data)'
if ($rate -lt $MinimumLineRate) { Say ("coverage: FAILED line coverage {0:P1} is below {1:P0}" -f $rate, $MinimumLineRate); exit 1 }
Write-Output ("COVERAGE_LINE={0:P1}" -f $rate)
exit 0
