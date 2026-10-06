# Quality gate for the local-only Ditto fork.
#   1. builds Release|x64 (restores NuGet packages first when packages\ is missing)
#   2. scans the imports of every built .exe/.dll for network DLLs
#   3. greps all sources for network APIs and network DLL names
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
    if (-not (Test-Path (Join-Path $repo 'packages'))) {
        Say 'restore: NuGet packages'
        & $msbuild $sln /t:restore /p:RestorePackagesConfig=true /nologo /v:q
        if ($LASTEXITCODE -ne 0) { Fail "restore exit $LASTEXITCODE" }
    }
    Say 'build: Release|x64'
    $start = Get-Date
    $out = & $msbuild $sln /p:Configuration=Release /p:Platform=x64 /p:VcpkgEnabled=false /m /nologo /v:m 2>&1
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
    $imports = @(& $dumpbin.FullName /nologo /imports $bin.FullName | Where-Object { $_ -match '^\s+(\S+\.dll)\s*$' } | ForEach-Object { $Matches[1].ToLowerInvariant() })
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
$sourceDirs = 'src', 'Shared', 'Addins', 'ICU_Loader', 'focusdll', 'EncryptDecrypt', 'FocusHighlight', 'U3Stop'
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

Say "VERIFY OK: build, imports ($($binaries.Count) binaries), source ($($files.Count) files)"
exit 0
