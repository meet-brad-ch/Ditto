# Writes a Markdown summary of the unit-test run (build\test-results, written by verify.ps1 stage 7)
# to -Output, e.g. $env:GITHUB_STEP_SUMMARY in CI: one table row per test, failures first, with the
# failure message. A test listed in tests.txt without an XML result (e.g. it crashed) is a failure.
# Exit code: 0 when every expected test passed, 1 otherwise or when there are no results.
param(
    [Parameter(Mandatory)] [string] $Results,
    [Parameter(Mandatory)] [string] $Output
)
$ErrorActionPreference = 'Stop'

$expectedFile = Join-Path $Results 'tests.txt'
if (-not (Test-Path $expectedFile)) {
    Add-Content -Path $Output -Value "## Unit tests`n`n**No test results**: $expectedFile is missing (the test stage did not run)."
    exit 1
}
$rows = foreach ($name in [IO.File]::ReadAllLines($expectedFile)) {
    $xmlPath = Join-Path $Results "$name.xml"
    if (-not (Test-Path $xmlPath)) {
        [pscustomobject]@{ Name = $name; Passed = $false; Time = ''; Detail = 'no result file (the test process crashed or was killed)' }
        continue
    }
    [xml]$xml = [IO.File]::ReadAllText($xmlPath)
    $case = $xml.testsuites.testsuite.testcase | Select-Object -First 1
    $failure = @($case.failure)[0]
    $detail = if ($failure) { (($failure.message -split "`n") | Select-Object -First 3) -join ' / ' } else { '' }
    $seconds = [double]::Parse("$($case.time)".TrimEnd('.'), [Globalization.CultureInfo]::InvariantCulture)
    [pscustomobject]@{ Name = $name; Passed = (-not $failure); Time = ('{0:N3} s' -f $seconds); Detail = $detail }
}
$rows = @($rows)
$failed = @($rows | Where-Object { -not $_.Passed })
$lines = [Collections.Generic.List[string]]::new()
$lines.Add('## Unit tests')
$lines.Add('')
$lines.Add(("**{0} of {1} passed**, each test in its own process under AddressSanitizer." -f ($rows.Count - $failed.Count), $rows.Count))
$lines.Add('')
$lines.Add('| Result | Test | Time | Detail |')
$lines.Add('|---|---|---|---|')
foreach ($r in ($failed + @($rows | Where-Object { $_.Passed }))) {
    $mark = if ($r.Passed) { 'pass' } else { '**FAIL**' }
    $lines.Add(('| {0} | `{1}` | {2} | {3} |' -f $mark, $r.Name, $r.Time, ($r.Detail -replace '\|', '\|')))
}
Add-Content -Path $Output -Value ($lines -join "`n")
if ($failed.Count -gt 0) { exit 1 }
exit 0
