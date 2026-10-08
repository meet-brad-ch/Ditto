# Quality gate for the local-only Ditto fork.
#   1. installs vcpkg.json dependencies, then rebuilds Release|x64 with a build log
#      - warnings: none (every project builds with /W4 /WX; the log is checked too)
#      - analyze (-Analyze): no code analysis finding (the build log check fails on any)
#   2. scans the imports of every built .exe/.dll for network DLLs
#   3. checks every built binary for ASLR, DEP and Control Flow Guard
#   4. greps all sources for network APIs and network DLL names
#   5. checks every installer script for firewall rules, URL launches and a Windows 10 minimum
#   6. raw allocation (new/delete/malloc/free) and globals (free functions, global/static variables,
#      macros): none in contract code; legacy code held by shrink-only baselines
#      (tools\gates\allocation.ps1, globals.ps1; untouched third-party code: tools\thirdparty.txt)
#      - complexity: every function of the own code below CC 10 (tools\gates\complexity.ps1)
#      - formatting: every own C/C++ file matches .clang-format (tools\gates\format.ps1)
#   7. runs every unit test on its own: DittoTests (AddressSanitizer build) and AppTests
#      - coverage: line coverage of lib\DittoCore >= 90 % (Debug|x64 test build); the app layer's
#        coverage (the src\ files AppTests compiles) is reported without a minimum
#   8. checks with Doxygen that the contract code is fully documented
# Prints one timestamped line per check and exits 1 on the first failed stage.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 [-SkipBuild] [-UpdateBaselines]
#   -SkipBuild        reuse the last build (the warnings checks are then NOT VERIFIED)
#   -Analyze          run MSVC code analysis (/analyze, NativeRecommendedRules) in the rebuild; any
#                     finding fails the build (about 7 min)
#   -UpdateBaselines  rewrite the allocation and globals baselines after a passing run; they may only shrink
param([switch] $SkipBuild, [switch] $Analyze, [switch] $UpdateBaselines)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Say([string] $msg) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $msg" }

# The sw-quality section 38 Verification block: every line starts NOT VERIFIED and is set by its stage.
$report = [ordered]@{
    'Build'                 = 'NOT VERIFIED (stage did not run)'
    'Unit tests'            = 'NOT VERIFIED (stage did not run)'
    'Integration tests'     = 'N/A (the project has no integration test suite)'
    'Lint'                  = 'NOT VERIFIED (stage did not run)'
    'Formatting'            = 'NOT VERIFIED (no formatter is set up)'
    'Static type check'     = 'N/A (C++: the compiler is the type checker)'
    'Static analysis'       = 'NOT VERIFIED (stage did not run)'
    'Cyclomatic complexity' = 'NOT VERIFIED (stage did not run)'
    'Line coverage'         = 'NOT VERIFIED (stage did not run)'
    'Branch coverage'       = 'NOT VERIFIED (Microsoft code coverage reports no branch data)'
}
function Write-Report {
    Write-Output ''
    Write-Output 'Verification (sw-quality section 38)'
    foreach ($k in $report.Keys) { Write-Output ("  {0,-22} {1}" -f "${k}:", $report[$k]) }
}
# Runs tools\gates\<script>, passes its output through, and returns its exit code plus the
# ratchet summary ("total N (baseline M)") when it printed one.
function Invoke-Gate([string] $script, [hashtable] $arguments) {
    $lines = @(& (Join-Path $PSScriptRoot "gates\$script") @arguments)
    $exitCode = $LASTEXITCODE
    $lines | ForEach-Object { Write-Output $_ } | Out-Host
    $summary = ($lines | Where-Object { "$_" -match '(total \d+ \(baseline \d+\)|baseline (written|created).*)' } | Select-Object -Last 1)
    $summary = if ($summary -and "$summary" -match '(total \d+ \(baseline \d+\)|baseline (written|created).*)') { $Matches[1] } else { '' }
    return [pscustomobject]@{ ExitCode = $exitCode; Summary = $summary; Lines = $lines }
}
function Fail([string] $msg, [string] $line = '') {
    if ($line) { $report[$line] = "FAIL ($msg)" }
    Write-Output "$(Get-Date -Format 'HH:mm:ss') FAILED $msg"
    Write-Report
    exit 1
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { Fail "vswhere.exe not found at $vswhere" }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
if (-not $vs) { Fail 'no Visual Studio with the MFC component (Microsoft.VisualStudio.Component.VC.ATLMFC)' }
$msbuild = Join-Path $vs 'MSBuild\Current\Bin\amd64\MSBuild.exe'
$dumpbin = Get-ChildItem (Join-Path $vs 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') | Sort-Object FullName | Select-Object -Last 1
if (-not $dumpbin) { Fail "dumpbin.exe not found under $vs" }

# ---- 1. build ---------------------------------------------------------------
$sln = Join-Path $repo 'CP_Main_10.sln'
$logDir = Join-Path $repo 'Release64'
if (-not $SkipBuild) {
    # one install up front: under /m every project would otherwise start its own vcpkg install
    $vcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { 'C:\vcpkg' }
    $vcpkg = Join-Path $vcpkgRoot 'vcpkg.exe'
    if (-not (Test-Path $vcpkg)) { Fail "vcpkg.exe not found at $vcpkg (set VCPKG_ROOT)" }
    Say 'vcpkg: install from vcpkg.json (x64-windows-static-md)'
    & $vcpkg install --triplet x64-windows-static-md "--x-manifest-root=$repo" "--x-install-root=$(Join-Path $repo 'vcpkg_installed\x64-windows-static-md')" --no-print-usage | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "vcpkg install exit $LASTEXITCODE" }
    # a full rebuild: an incremental build reports only the warnings of the files it recompiles.
    # Headers generated by #import (*.tlh, *.tli) survive /t:Rebuild; delete them so the rebuild
    # regenerates them and reports their warnings, exactly like a fresh clone does.
    Get-ChildItem (Join-Path $repo 'Release64') -Recurse -File -Include *.tlh, *.tli -ErrorAction SilentlyContinue |
        ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
    $buildLogDir = Join-Path $repo 'build\logs'
    New-Item -ItemType Directory $buildLogDir -Force | Out-Null
    $buildLog = Join-Path $buildLogDir 'build.log'
    $buildArgs = @($sln, '/t:Rebuild', '/p:Configuration=Release', '/p:Platform=x64', "/p:VcpkgRoot=$vcpkgRoot", '/m', '/nologo', '/v:m', "/flp:logfile=$buildLog;verbosity=normal")
    if ($Analyze) {
        # code analysis runs inside the same rebuild; it does not change the generated code
        $buildArgs += '/p:RunCodeAnalysis=true', '/p:EnableMicrosoftCodeAnalysis=true', '/p:EnableClangTidyCodeAnalysis=false', '/p:CodeAnalysisRuleSet=NativeRecommendedRules.ruleset'
        Say 'build: Release|x64 (rebuild with /analyze)'
    }
    else { Say 'build: Release|x64 (rebuild)' }
    $start = Get-Date
    $out = & $msbuild @buildArgs 2>&1
    $code = $LASTEXITCODE
    $errors = @($out | Where-Object { "$_" -match ': (fatal )?error ' })
    $errors | Select-Object -First 30 | ForEach-Object { Say "  $_" }
    if ($code -ne 0) { Fail "build exit $code" 'Build' }
    $outputs = @($out | Where-Object { "$_" -match '\.vcxproj -> ' })
    if ($outputs.Count -eq 0) { Fail 'build reported no project outputs' 'Build' }
    $outputs | ForEach-Object { Say ("  " + ("$_".Trim() -replace '^.*\\([^\\]+\.vcxproj) -> ', '$1 -> ')) }
    $minutes = ((Get-Date) - $start).TotalMinutes
    Say ("build: OK in {0:N1} min" -f $minutes)
    $report['Build'] = "PASS (Release|x64 rebuild, {0} projects, {1:N1} min)" -f $outputs.Count, $minutes

    # Every project builds with /W4 /WX (Directory.Build.targets): a compiler warning already
    # failed the build above. /WX does not make every code analysis finding an error (C26495
    # findings left the build passing, 2026-10-07), so under -Analyze this log check is what
    # fails on a finding, as on any compiler or linker warning a project could still let through.
    $warnings = @([IO.File]::ReadLines($buildLog) | Where-Object { $_ -match ':\s+(?:command line\s+)?warning\s+(?:C|LNK)\d+\s*:' } |
        ForEach-Object { ($_ -replace '^\s*\d+>', '').Trim() } | Sort-Object -Unique)
    if ($warnings.Count -gt 0) {
        $warnings | Select-Object -First 30 | ForEach-Object { Say "  $_" }
        Fail "$($warnings.Count) warnings in the build log" 'Lint'
    }
    Say 'warnings: ok   none (/W4 /WX in every project)'
    $report['Lint'] = 'PASS (zero compiler and linker warnings, /W4 /WX)'
    if ($Analyze) { $report['Static analysis'] = 'PASS (zero /analyze NativeRecommendedRules findings in the build log)' }
    else { $report['Static analysis'] = 'NOT VERIFIED (run with -Analyze)' }
}
else {
    $report['Build'] = 'NOT VERIFIED (-SkipBuild: last build reused)'
    $report['Lint'] = 'NOT VERIFIED (-SkipBuild: no fresh build)'
    $report['Static analysis'] = 'NOT VERIFIED (-SkipBuild)'
}

# ---- 2. import scan ----------------------------------------------------------
$bannedDlls = 'ws2_32', 'wsock32', 'mswsock', 'wininet', 'winhttp', 'urlmon', 'mapi32', 'dnsapi', 'iphlpapi', 'webio', 'httpapi'
$binaries = @(Get-ChildItem (Join-Path $repo 'Release64') -Recurse -Include *.exe, *.dll)
if ($binaries.Count -eq 0) { Fail 'no binaries under Release64 (run without -SkipBuild)' }
$importFindings = 0
foreach ($bin in $binaries) {
    $dump = & $dumpbin.FullName /nologo /imports $bin.FullName
    if ($LASTEXITCODE -ne 0) { Fail "imports: dumpbin exit $LASTEXITCODE on $($bin.FullName)" }
    $imports = @($dump | Where-Object { $_ -match '^\s+(\S+\.dll)\s*$' } | ForEach-Object { $Matches[1].ToLowerInvariant() })
    $bad = @($imports | Where-Object { $bannedDlls -contains [IO.Path]::GetFileNameWithoutExtension($_) })
    $rel = $bin.FullName.Substring($repo.Length + 1)
    if ($bad.Count -gt 0) { Say "imports: FAIL $rel imports $($bad -join ', ')"; $importFindings++ }
    else { Say "imports: ok   $rel ($($imports.Count) DLLs)" }
}
if ($importFindings -gt 0) { Fail "imports: $importFindings binaries import network DLLs" }

# ---- 3. hardening --------------------------------------------------------------
# Every binary must carry ASLR (64-bit), DEP and Control Flow Guard in its DLL characteristics.
$requiredFlags = 'High Entropy Virtual Addresses', 'Dynamic base', 'NX compatible', 'Control Flow Guard'   # dumpbin /headers wording
$hardeningFindings = 0
foreach ($bin in $binaries) {
    $dump = & $dumpbin.FullName /nologo /headers $bin.FullName
    if ($LASTEXITCODE -ne 0) { Fail "hardening: dumpbin exit $LASTEXITCODE on $($bin.FullName)" }
    $flags = @($dump | ForEach-Object { "$_".Trim() })
    $missing = @($requiredFlags | Where-Object { $flags -notcontains $_ })
    $rel = $bin.FullName.Substring($repo.Length + 1)
    if ($missing.Count -gt 0) { Say "hardening: FAIL $rel lacks $($missing -join ', ')"; $hardeningFindings++ }
    else { Say "hardening: ok   $rel (high-entropy ASLR, DEP, CFG)" }
}
if ($hardeningFindings -gt 0) { Fail "hardening: $hardeningFindings binaries lack ASLR/DEP/CFG flags" }

# ---- 4. source grep ------------------------------------------------------------
# Case-sensitive API names, matched as whole words; DLL names case-insensitive.
$apiPattern = '\b(WSAStartup|WSASocket|closesocket|gethostbyname|getaddrinfo|GetAddrInfoW|InternetOpen\w*|InternetConnect\w*|InternetCanonicalizeUrl|HttpOpenRequest\w*|WinHttp\w+|URLDownloadTo\w+|URLOpenStream\w*|MAPISendMail\w*|CLSID_WebBrowser|IWebBrowser2?|CAsyncSocket|CSocket|GotoURL)\b'
$dllPattern = '(?i)\b(ws2_32|wsock32|wininet|winhttp|urlmon|mapi32|dnsapi|iphlpapi)\b'
$urlLaunchPattern = '(?i)ShellExecute\w*\s*\(.*https?://'
$sourceDirs = 'src', 'Shared', 'Addins', 'ICU_Loader', 'focusdll', 'FocusHighlight', 'lib', 'tests'
$files = foreach ($d in $sourceDirs) {
    $p = Join-Path $repo $d
    if (Test-Path $p) { Get-ChildItem $p -Recurse -File -Include *.c, *.cpp, *.h, *.hpp, *.rc, *.vcxproj, *.def }
}
$files = @($files) + @(Get-ChildItem (Join-Path $repo '*') -File -Include *.rc, *.vcxproj)
$sourceFindings = 0
foreach ($f in $files) {
    $n = 0
    foreach ($line in [IO.File]::ReadLines($f.FullName)) {
        $n++
        if ($line -cmatch $apiPattern -or $line -match $dllPattern -or $line -match $urlLaunchPattern) {
            Say ("source: FAIL {0}:{1}: {2}" -f $f.FullName.Substring($repo.Length + 1), $n, $line.Trim())
            $sourceFindings++
        }
    }
}
if ($sourceFindings -gt 0) { Fail "source: $sourceFindings lines reference network APIs or DLLs" }
Say "source: ok   $($files.Count) files, no network APIs or DLL names"

# ---- 5. installer scripts --------------------------------------------------------
# Every Inno Setup script in the repo: no firewall rules, no post-install URL launches, and a
# MinVersion of Windows 10 or later.
$issFiles = @(Get-ChildItem $repo -Recurse -Filter *.iss | Where-Object { $_.FullName.Substring($repo.Length + 1) -notmatch '^(build|vcpkg_installed|Release64)\\' })
if ($issFiles.Count -eq 0) { Fail 'installer: no .iss script found' }
$issPattern = '(?i)\b(netsh|advfirewall|firewall)\b|^\s*Filename:\s*https?://|\bshellexec\b'
$issFindings = 0
foreach ($iss in $issFiles) {
    $rel = $iss.FullName.Substring($repo.Length + 1)
    $n = 0
    $minVersionOk = $false
    foreach ($line in [IO.File]::ReadLines($iss.FullName)) {
        $n++
        if ($line.TrimStart().StartsWith(';') -or $line.TrimStart().StartsWith('//')) { continue }   # comments
        if ($line -match $issPattern) { Say "installer: FAIL ${rel}:${n}: $($line.Trim())"; $issFindings++ }
        if ($line -match '^\s*MinVersion\s*=\s*(\d+)\.' -and [int]$Matches[1] -ge 10) { $minVersionOk = $true }
    }
    if (-not $minVersionOk) { Say "installer: FAIL $rel has no MinVersion of Windows 10 or later"; $issFindings++ }
}
if ($issFindings -gt 0) { Fail "installer: $issFindings findings (firewall rules, URL launches or OS version)" }
Say "installer: ok   $($issFiles.Count) script(s): no firewall rules or URL launches, Windows 10 or later"

# ---- 6. no raw allocation, no globals, no macros ----------------------------------------
# Owner rules: smart pointers and containers only; behaviour in classes, no global state; avoid
# macros. Contract code (lib\, tests\) has none; legacy code is held by shrink-only baselines
# (tools\baselines\allocation.tsv, globals.tsv). Untouched third-party code (tools\thirdparty.txt)
# is skipped by every gate.
$gateArgs = @{ Repo = $repo }
if ($UpdateBaselines) { $gateArgs['Update'] = $true }
$gate = Invoke-Gate 'allocation.ps1' $gateArgs
if ($gate.ExitCode -ne 0) { Fail 'raw allocation above the baseline or in contract code' 'Lint' }
$allocationSummary = $gate.Summary
$gate = Invoke-Gate 'globals.ps1' $gateArgs
if ($gate.ExitCode -ne 0) { Fail 'a global, free function or macro above the baseline or in lib\' 'Lint' }
$globalsSummary = $gate.Summary
$report['Lint'] += "; raw allocation: none in contract code, legacy $allocationSummary; globals/free functions/macros: none in lib\, legacy $globalsSummary"

# ---- 6b. cyclomatic complexity (lizard) -----------------------------------------------
# Every function of Ditto's own code stays below CC 10 (no baseline since Phase L2).
$gate = Invoke-Gate 'complexity.ps1' @{ Repo = $repo }
if ($gate.ExitCode -ne 0) { Fail 'a function is at or above complexity 10' 'Cyclomatic complexity' }
$measured = ($gate.Lines | Where-Object { "$_" -match '(\d+) functions in (\d+) files; max CC (\d+)' } | Select-Object -First 1)
$null = "$measured" -match '(\d+) functions in (\d+) files; max CC (\d+)'
$report['Cyclomatic complexity'] = "PASS (all own code: $($Matches[1]) functions in $($Matches[2]) files, max CC $($Matches[3]) < 10)"

# ---- 6c. formatting (clang-format) -------------------------------------------------------
# Every own C/C++ file matches .clang-format (owner decision 2026-10-07; fix with
# tools\gates\format.ps1 -Repo . -Fix).
$gate = Invoke-Gate 'format.ps1' @{ Repo = $repo }
if ($gate.ExitCode -ne 0) { Fail 'a file differs from .clang-format' 'Formatting' }
$formatVersion = ($gate.Lines | Where-Object { "$_" -match 'format: (clang-format version [\d.]+)' } | Select-Object -First 1)
$null = "$formatVersion" -match 'format: (clang-format version [\d.]+)'
$formatTool = $Matches[1]
$formatCount = ($gate.Lines | Where-Object { "$_" -match 'format: (\d+) files; 0 not formatted' } | Select-Object -First 1)
$null = "$formatCount" -match 'format: (\d+) files'
$report['Formatting'] = "PASS ($($Matches[1]) files match .clang-format, $formatTool)"

# ---- 7. unit tests (each test on its own) ------------------------------------------------
# DittoTests (lib\DittoCore) is an AddressSanitizer build; AppTests (the app layer against an
# in-memory database) is MFC and runs without ASan. Suite names differ between the programs, so
# the result files (one per test, named after it) do not collide.
$testPrograms = @(
    [pscustomobject]@{ Name = 'DittoTests'; Exe = (Join-Path $repo 'build\DittoTests\x64\Release\DittoTests.exe') },
    [pscustomobject]@{ Name = 'AppTests'; Exe = (Join-Path $repo 'build\AppTests\x64\Release\AppTests.exe') }
)
$env:PATH = "$($dumpbin.DirectoryName);$env:PATH"   # clang_rt.asan_dynamic-x86_64.dll lives next to dumpbin
$resultsDir = Join-Path $repo 'build\test-results'   # one GoogleTest XML per test, for the CI summary
if (Test-Path $resultsDir) { Remove-Item -LiteralPath $resultsDir -Recurse -Force }
New-Item -ItemType Directory $resultsDir | Out-Null
$testNames = @()
$testFailures = 0
foreach ($program in $testPrograms) {
    if (-not (Test-Path $program.Exe)) { Fail "tests: $($program.Exe) not found (run without -SkipBuild)" }
    $suite = ''
    $names = @(& $program.Exe --gtest_list_tests | ForEach-Object {
        if ($_ -match '^(\w+)\.$') { $suite = $Matches[1] } elseif ($_ -match '^\s+(\w+)') { "$suite.$($Matches[1])" } })
    if ($LASTEXITCODE -ne 0 -or $names.Count -eq 0) { Fail "tests: could not list the tests of $($program.Name) (exit $LASTEXITCODE)" }
    foreach ($name in $names) {
        $out = & $program.Exe "--gtest_filter=$name" --gtest_brief=1 "--gtest_output=xml:$(Join-Path $resultsDir "$name.xml")" 2>&1 | Out-String
        if ($LASTEXITCODE -ne 0) {
            Say "tests: FAIL $name ($($program.Name))"
            ($out -split "`n" | Where-Object { $_ -match 'error|Failure|AddressSanitizer' } | Select-Object -First 5) | ForEach-Object { Say "    $($_.Trim())" }
            $testFailures++
        }
        else { Say "tests: PASS $name" }
    }
    Say "tests: $($program.Name) ran $($names.Count) tests"
    $testNames += $names
}
[IO.File]::WriteAllLines((Join-Path $resultsDir 'tests.txt'), [string[]]$testNames)   # the expected set
if ($testFailures -gt 0) { Fail "$testFailures of $($testNames.Count) tests failed" 'Unit tests' }
Say "tests: ok   $($testNames.Count) tests, each run on its own (DittoTests under ASan)"
$report['Unit tests'] = "PASS ($($testNames.Count)/$($testNames.Count), each test in its own process; DittoTests in an AddressSanitizer build)"

# ---- 7b. coverage (Microsoft code coverage, Debug|x64 test build) -------------------------
$gate = Invoke-Gate 'coverage.ps1' @{ Repo = $repo; VsPath = $vs }
if ($gate.ExitCode -ne 0) { Fail 'line coverage of lib\DittoCore below 90 % or not measurable' 'Line coverage' }
$lineRate = ($gate.Lines | Where-Object { "$_" -match '^COVERAGE_LINE=' } | Select-Object -First 1) -replace '^COVERAGE_LINE=', ''
$appRate = ($gate.Lines | Where-Object { "$_" -match '^APP_COVERAGE_LINE=' } | Select-Object -First 1) -replace '^APP_COVERAGE_LINE=', ''
$report['Line coverage'] = "PASS ($lineRate of lib\DittoCore, minimum 90 %; app layer (AppTests): $appRate, reported)"

# ---- 8. documentation (Doxygen) -------------------------------------------------------
# Every class, function and member of the contract code is documented; any Doxygen warning fails.
$doxygen = (Get-Command doxygen -ErrorAction SilentlyContinue).Source
if (-not $doxygen -and (Test-Path 'C:\Program Files\doxygen\bin\doxygen.exe')) { $doxygen = 'C:\Program Files\doxygen\bin\doxygen.exe' }
if (-not $doxygen) { Fail 'docs: doxygen not found (install Doxygen or put it on PATH)' }
Push-Location $repo
try {
    $docOut = & $doxygen (Join-Path $repo 'tools\Doxyfile.contract') 2>&1
    $docCode = $LASTEXITCODE
}
finally { Pop-Location }
$docOut | Where-Object { "$_".Trim() } | Select-Object -First 20 | ForEach-Object { Say "  docs: $_" }
if ($docCode -ne 0) { Fail "docs: doxygen exit $docCode (undocumented or wrongly documented code)" }
Say 'docs: ok   contract code fully documented (tools\Doxyfile.contract)'

Say "VERIFY OK: build, imports and hardening ($($binaries.Count) binaries), source ($($files.Count) files), installer scripts ($($issFiles.Count)), allocation, globals, complexity, tests ($($testNames.Count)), coverage, docs"
Write-Report
exit 0
