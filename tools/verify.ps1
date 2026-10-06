# Quality gate for the local-only Ditto fork.
#   1. installs vcpkg.json dependencies, then builds Release|x64
#   2. scans the imports of every built .exe/.dll for network DLLs
#   3. greps all sources for network APIs and network DLL names
#   4. checks the installer script for firewall rules and URL launches
#   5. rejects raw allocation (new/delete/malloc/free) in lib\ and tests\
#   6. runs every unit test on its own (AddressSanitizer build)
#   7. checks with Doxygen that the contract code is fully documented
# Prints one timestamped line per check and exits 1 on the first failed stage.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 [-SkipBuild]
param([switch] $SkipBuild)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Say([string] $msg) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $msg" }
function Fail([string] $msg) { Write-Output "$(Get-Date -Format 'HH:mm:ss') FAILED $msg"; exit 1 }

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
    Say 'build: Release|x64'
    $start = Get-Date
    $out = & $msbuild $sln /p:Configuration=Release /p:Platform=x64 "/p:VcpkgRoot=$vcpkgRoot" /m /nologo /v:m 2>&1
    $code = $LASTEXITCODE
    $errors = @($out | Where-Object { "$_" -match ': (fatal )?error ' })
    $errors | Select-Object -First 30 | ForEach-Object { Say "  $_" }
    if ($code -ne 0) { Fail "build exit $code" }
    $outputs = @($out | Where-Object { "$_" -match '\.vcxproj -> ' })
    if ($outputs.Count -eq 0) { Fail 'build reported no project outputs' }
    $outputs | ForEach-Object { Say ("  " + ("$_".Trim() -replace '^.*\\([^\\]+\.vcxproj) -> ', '$1 -> ')) }
    Say ("build: OK in {0:N1} min" -f ((Get-Date) - $start).TotalMinutes)
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

# ---- 3. source grep ------------------------------------------------------------
# Case-sensitive API names, matched as whole words; DLL names case-insensitive.
$apiPattern = '\b(WSAStartup|WSASocket|closesocket|gethostbyname|getaddrinfo|GetAddrInfoW|InternetOpen\w*|InternetConnect\w*|InternetCanonicalizeUrl|HttpOpenRequest\w*|WinHttp\w+|URLDownloadTo\w+|URLOpenStream\w*|MAPISendMail\w*|CLSID_WebBrowser|IWebBrowser2?|CAsyncSocket|CSocket|GotoURL)\b'
$dllPattern = '(?i)\b(ws2_32|wsock32|wininet|winhttp|urlmon|mapi32|dnsapi|iphlpapi)\b'
$urlLaunchPattern = '(?i)ShellExecute\w*\s*\(.*https?://'
$sourceDirs = 'src', 'Shared', 'Addins', 'ICU_Loader', 'focusdll', 'EncryptDecrypt', 'FocusHighlight', 'U3Stop', 'lib', 'tests'
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

# ---- 4. installer script ---------------------------------------------------------
# No firewall rules and no post-install URL launches in the installer the fork builds.
$iss = Join-Path $repo 'DittoSetup\DittoSetup_10.iss'
$issPattern = '(?i)\b(netsh|advfirewall|firewall)\b|^\s*Filename:\s*https?://|\bshellexec\b'
$issFindings = 0
$n = 0
foreach ($line in [IO.File]::ReadLines($iss)) {
    $n++
    if ($line.TrimStart().StartsWith(';') -or $line.TrimStart().StartsWith('//')) { continue }   # comments
    if ($line -match $issPattern) { Say "installer: FAIL DittoSetup_10.iss:${n}: $($line.Trim())"; $issFindings++ }
}
if ($issFindings -gt 0) { Fail "installer: $issFindings lines add firewall rules or launch URLs" }
Say 'installer: ok   DittoSetup_10.iss has no firewall rules or URL launches'

# ---- 5. no raw allocation in contract code ------------------------------------------
# Owner rule: no new/delete/malloc/free; smart pointers and containers only. Deleted functions
# ('= delete') and comments are not allocations.
$allocPattern = '\bnew\b|\bdelete\b|\b(malloc|calloc|realloc|free)\s*\('
$contractFiles = @(Get-ChildItem (Join-Path $repo 'lib'), (Join-Path $repo 'tests') -Recurse -File -Include *.cpp, *.h)
$allocFindings = 0
foreach ($f in $contractFiles) {
    $n = 0
    foreach ($line in [IO.File]::ReadLines($f.FullName)) {
        $n++
        $code = ($line -replace '//.*$', '') -replace '=\s*delete\b', ''
        if ($code -cmatch $allocPattern) {
            Say ("allocation: FAIL {0}:{1}: {2}" -f $f.FullName.Substring($repo.Length + 1), $n, $line.Trim())
            $allocFindings++
        }
    }
}
if ($allocFindings -gt 0) { Fail "allocation: $allocFindings raw allocations in lib\ or tests\" }
Say "allocation: ok   $($contractFiles.Count) files in lib\ and tests\, no raw new/delete/malloc/free"

# ---- 6. unit tests (each test on its own, AddressSanitizer build) -----------------------
$testExe = Join-Path $repo 'build\DittoTests\x64\Release\DittoTests.exe'
if (-not (Test-Path $testExe)) { Fail "tests: $testExe not found (run without -SkipBuild)" }
$env:PATH = "$($dumpbin.DirectoryName);$env:PATH"   # clang_rt.asan_dynamic-x86_64.dll lives next to dumpbin
$suite = ''
$testNames = @(& $testExe --gtest_list_tests | ForEach-Object {
    if ($_ -match '^(\w+)\.$') { $suite = $Matches[1] } elseif ($_ -match '^\s+(\w+)') { "$suite.$($Matches[1])" } })
if ($LASTEXITCODE -ne 0 -or $testNames.Count -eq 0) { Fail "tests: could not list tests (exit $LASTEXITCODE)" }
$testFailures = 0
foreach ($name in $testNames) {
    $out = & $testExe "--gtest_filter=$name" --gtest_brief=1 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        Say "tests: FAIL $name"
        ($out -split "`n" | Where-Object { $_ -match 'error|Failure|AddressSanitizer' } | Select-Object -First 5) | ForEach-Object { Say "    $($_.Trim())" }
        $testFailures++
    }
    else { Say "tests: PASS $name" }
}
if ($testFailures -gt 0) { Fail "tests: $testFailures of $($testNames.Count) failed" }
Say "tests: ok   $($testNames.Count) tests, each run on its own under ASan"

# ---- 7. documentation (Doxygen) -------------------------------------------------------
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

Say "VERIFY OK: build, imports ($($binaries.Count) binaries), source ($($files.Count) files), installer script, allocation, tests ($($testNames.Count)), docs"
exit 0
