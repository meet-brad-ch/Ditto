# Quality gate for the local-only Ditto fork.
#   1. installs vcpkg.json dependencies, then rebuilds Release|x64 with a build log
#      - warnings: the build's warnings may not exceed tools\baselines\warnings.tsv
#      - analyze (-Analyze): code analysis findings may not exceed tools\baselines\analyze.tsv
#   2. scans the imports of every built .exe/.dll for network DLLs
#   3. checks every built binary for ASLR, DEP and Control Flow Guard
#   4. greps all sources for network APIs and network DLL names
#   5. checks every installer script for firewall rules, URL launches and a Windows 10 minimum
#   6. rejects raw allocation (new/delete/malloc/free) in lib\ and tests\
#      - complexity: lib\ and tests\ below CC 10; legacy CC >= 10 held by tools\baselines\complexity.tsv
#   7. runs every unit test on its own: DittoTests (AddressSanitizer build) and AppTests
#      - coverage: line coverage of lib\DittoCore >= 90 % (Debug|x64 test build)
#   8. checks with Doxygen that the contract code is fully documented
# Prints one timestamped line per check and exits 1 on the first failed stage.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 [-SkipBuild] [-UpdateBaselines]
#   -SkipBuild        reuse the last build (the warnings gates are then NOT VERIFIED)
#   -Analyze          run MSVC code analysis (/analyze, NativeRecommendedRules) in the rebuild and
#                     ratchet its findings against tools\baselines\analyze.tsv (about 7 min)
#   -UpdateBaselines  rewrite the ratchet baselines after a passing run; they may only shrink
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

    $warningArgs = @{ Repo = $repo; Log = $buildLog; Kind = 'build' }
    if ($UpdateBaselines) { $warningArgs['Update'] = $true }
    $gate = Invoke-Gate 'warnings.ps1' $warningArgs
    if ($gate.ExitCode -ne 0) { Fail 'warnings above the baseline' 'Lint' }
    $report['Lint'] = "PASS (/W4 warnings ratchet: $($gate.Summary))"
    if ($Analyze) {
        $warningArgs['Kind'] = 'analyze'
        $gate = Invoke-Gate 'warnings.ps1' $warningArgs
        if ($gate.ExitCode -ne 0) { Fail 'code analysis findings above the baseline' 'Static analysis' }
        $report['Static analysis'] = "PASS (/analyze NativeRecommendedRules ratchet: $($gate.Summary))"
    }
    else { $report['Static analysis'] = 'NOT VERIFIED (run with -Analyze)' }
}
else {
    $report['Build'] = 'NOT VERIFIED (-SkipBuild: last build reused)'
    $report['Lint'] = 'NOT VERIFIED (-SkipBuild: no fresh build log)'
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

# ---- 6. no raw allocation in contract code ------------------------------------------
# Owner rule: no new/delete/malloc/free; smart pointers and containers only. Deleted functions
# ('= delete'), comments and the text of string and character literals are not allocations.
$allocPattern = '\bnew\b|\bdelete\b|\b(malloc|calloc|realloc|free)\s*\('
$contractFiles = @(Get-ChildItem (Join-Path $repo 'lib'), (Join-Path $repo 'tests') -Recurse -File -Include *.cpp, *.h)
$allocFindings = 0
foreach ($f in $contractFiles) {
    $n = 0
    $inBlock = $false   # inside a /* ... */ (or Doxygen /** ... */) comment
    foreach ($line in [IO.File]::ReadLines($f.FullName)) {
        $n++
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
        # text in string and character literals is not code ("new shequel" in the slug table)
        $code = $code -replace '"(\\.|[^"\\])*"', '""' -replace "'(\\.|[^'\\])*'", "''"
        if ($code -cmatch $allocPattern) {
            Say ("allocation: FAIL {0}:{1}: {2}" -f $f.FullName.Substring($repo.Length + 1), $n, $line.Trim())
            $allocFindings++
        }
    }
}
if ($allocFindings -gt 0) { Fail "allocation: $allocFindings raw allocations in lib\ or tests\" }
Say "allocation: ok   $($contractFiles.Count) files in lib\ and tests\, no raw new/delete/malloc/free"

# ---- 6b. cyclomatic complexity (lizard) -----------------------------------------------
# Contract code stays below CC 10; legacy functions at CC >= 10 may not grow (baseline ratchet).
$complexityArgs = @{ Repo = $repo }
if ($UpdateBaselines) { $complexityArgs['Update'] = $true }
$gate = Invoke-Gate 'complexity.ps1' $complexityArgs
if ($gate.ExitCode -ne 0) { Fail 'a function is above its complexity limit or baseline' 'Cyclomatic complexity' }
$measured = ($gate.Lines | Where-Object { "$_" -match 'contract code max CC (\d+); (\d+) legacy' } | Select-Object -First 1)
$null = "$measured" -match 'contract code max CC (\d+); (\d+) legacy'
$report['Cyclomatic complexity'] = "PASS (lib\ and tests\ max CC $($Matches[1]) < 10; $($Matches[2]) legacy functions at CC >= 10 held by the baseline, $($gate.Summary))"

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
$report['Line coverage'] = "PASS ($lineRate of lib\DittoCore, minimum 90 %)"

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

Say "VERIFY OK: build, imports and hardening ($($binaries.Count) binaries), source ($($files.Count) files), installer scripts ($($issFiles.Count)), allocation, complexity, tests ($($testNames.Count)), coverage, docs"
Write-Report
exit 0
