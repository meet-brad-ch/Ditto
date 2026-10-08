# CI (the project's only CI; there is no GitHub workflow): run on this machine against a clean clone
# of one commit, so only committed files take part (no untracked or ignored leftovers).
#   1. clone the commit into build\ci\<commit>\work
#   2. tools\verify.ps1 -Analyze (rebuild with /analyze, all gates, every test alone under ASan)
#   3. tools\fuzz.ps1: every libFuzzer target for 60 s
#   4. Debug|x64, Debug|Win32 and Release|Win32 solution builds
#   5. the Inno Setup installer (an Inno Setup warning fails it)
# Writes build\ci\<commit>\summary.md (steps, the section 38 block, the per-test table, installer
# SHA256) and build\ci\<commit>\artifacts\ (installer, binaries, test XML, coverage, logs), then
# deletes the clone. Prints one timestamped line per step; exits 0 when every step passed.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1 [-Ref HEAD]
param([string] $Ref = 'HEAD')
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

$repo = Split-Path -Parent $PSScriptRoot
$sha = (git -C $repo rev-parse --verify "$Ref^{commit}").Trim()
if ($LASTEXITCODE -ne 0 -or -not $sha) { Say "ci: FAILED '$Ref' is not a commit"; exit 1 }
$short = $sha.Substring(0, 7)
$subject = (git -C $repo log -1 --format=%s $sha).Trim()
$dirty = @(git -C $repo status --porcelain --untracked-files=no)
if ($dirty.Count -gt 0) { Say "ci: note: $($dirty.Count) uncommitted changes in the working tree are NOT part of this run" }

$ciDir = Join-Path $repo "build\ci\$short"
if (Test-Path $ciDir) { Remove-Item -LiteralPath $ciDir -Recurse -Force }
$work = Join-Path $ciDir 'work'
$artifacts = Join-Path $ciDir 'artifacts'
New-Item -ItemType Directory $artifacts -Force | Out-Null
$summary = Join-Path $ciDir 'summary.md'
$steps = [Collections.Generic.List[string]]::new()
$start = Get-Date
$failed = $false

function Invoke-Step([string] $name, [scriptblock] $body) {
    $cell = $name -replace '\|', '\|'   # a literal | would split the Markdown table cell
    if ($script:failed) { $script:steps.Add("| $cell | skipped |"); Say "ci: SKIPPED $name"; return }
    Say "ci: $name"
    $t = Get-Date
    & $body
    $ok = $LASTEXITCODE -eq 0
    $minutes = ((Get-Date) - $t).TotalMinutes
    $script:steps.Add(("| {0} | {1} ({2:N1} min) |" -f $cell, $(if ($ok) { 'pass' } else { '**FAIL**' }), $minutes))
    if ($ok) { Say ("ci: ok   {0} ({1:N1} min)" -f $name, $minutes) }
    else { Say "ci: FAILED $name (exit $LASTEXITCODE)"; $script:failed = $true }
}

try {
    Invoke-Step "clean clone of $short" {
        git clone --quiet --no-hardlinks $repo $work
        if ($LASTEXITCODE -eq 0) { git -C $work checkout --quiet --detach $sha }
    }

    $verifyLog = Join-Path $artifacts 'verify.log'
    Invoke-Step 'verify -Analyze' {
        powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $work 'tools\verify.ps1') -Analyze *>&1 |
            Tee-Object -FilePath $verifyLog | Where-Object { "$_" -notmatch 'tests: PASS|imports: ok|hardening: ok|uncovered' } | ForEach-Object { "    $_" }
    }

    Invoke-Step 'fuzz (60 s per target)' {
        powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $work 'tools\fuzz.ps1') -Seconds 60 *>&1 | ForEach-Object { "    $_" }
    }

    # The other configurations (/W4 /WX like Release|x64): Debug code and 32-bit types show warnings
    # Release|x64 does not.
    Invoke-Step 'Debug|x64, Debug|Win32 and Release|Win32 builds' {
        $vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.ATLMFC -property installationPath
        $vcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { 'C:\vcpkg' }
        # one install up front: under /m every project would otherwise start its own vcpkg install
        & (Join-Path $vcpkgRoot 'vcpkg.exe') install --triplet x86-windows-static-md "--x-manifest-root=$work" "--x-install-root=$(Join-Path $work 'vcpkg_installed\x86-windows-static-md')" --no-print-usage | Out-Null
        $code = $LASTEXITCODE
        if ($code -ne 0) { "    vcpkg install x86-windows-static-md exit $code" }
        foreach ($config in @(@('Debug', 'x64'), @('Debug', 'Win32'), @('Release', 'Win32'))) {
            if ($code -ne 0) { break }
            $out = & (Join-Path $vs 'MSBuild\Current\Bin\amd64\MSBuild.exe') (Join-Path $work 'CP_Main_10.sln') "/p:Configuration=$($config[0])" "/p:Platform=$($config[1])" "/p:VcpkgRoot=$vcpkgRoot" /m /nologo /v:m 2>&1
            $code = $LASTEXITCODE
            "    $($config[0])|$($config[1]): exit $code"
            $out | Where-Object { "$_" -match ': (fatal )?error ' } | Select-Object -First 20 | ForEach-Object { "    $_" }
        }
        $global:LASTEXITCODE = $code
    }

    Invoke-Step 'installer' {
        $iscc = if ($env:INNO_DIR) { Join-Path $env:INNO_DIR 'ISCC.exe' } else { 'C:\Program Files\Inno Setup 7\ISCC.exe' }
        # not /Q: that hides the compiler's warnings, and a warning fails the step like a C++ one
        $out = & $iscc (Join-Path $work 'DittoSetup\DittoSetup_10.iss') 2>&1
        $code = $LASTEXITCODE
        $warnings = @($out | Where-Object { "$_" -match '^Warning:' })
        $warnings | ForEach-Object { "    $_" }
        if ($code -eq 0 -and $warnings.Count -gt 0) { "    $($warnings.Count) Inno Setup warnings"; $code = 1 }
        if ($code -ne 0) { $out | Where-Object { "$_" -match '^(Error|Line \d+)' } | ForEach-Object { "    $_" } }
        $global:LASTEXITCODE = $code
    }
}
finally {
    # results, whatever happened: the test table needs the clone's build\test-results
    $results = Join-Path $work 'build\test-results'
    $testTable = Join-Path $ciDir 'tests.md'
    if (Test-Path $results) {
        powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'tools\test-summary.ps1') -Results $results -Output $testTable | Out-Null
    }
    $copies = @(
        @{ From = (Join-Path $work 'DittoSetup\Output'); To = 'installer' },
        @{ From = (Join-Path $work 'Release64'); To = 'Release64' },
        @{ From = $results; To = 'test-results' },
        @{ From = (Join-Path $work 'build\coverage'); To = 'coverage' },
        @{ From = (Join-Path $work 'build\logs'); To = 'logs' },
        @{ From = (Join-Path $work 'build\fuzz'); To = 'fuzz' }
    )
    foreach ($c in $copies) {
        if (Test-Path $c.From) {
            $dest = Join-Path $artifacts $c.To
            if ($c.To -eq 'Release64') {
                Get-ChildItem $c.From -Recurse -File -Include *.exe, *.dll | ForEach-Object {
                    $rel = $_.FullName.Substring($c.From.Length + 1)
                    $target = Join-Path $dest $rel
                    New-Item -ItemType Directory (Split-Path -Parent $target) -Force | Out-Null
                    Copy-Item $_.FullName $target
                }
            }
            else { Copy-Item $c.From $dest -Recurse }
        }
    }

    $verifyLog = Join-Path $artifacts 'verify.log'
    $section = @()
    if (Test-Path $verifyLog) {
        $lines = [IO.File]::ReadAllLines($verifyLog)
        $at = [Array]::IndexOf($lines, ($lines | Where-Object { $_ -like 'Verification (sw-quality section 38)*' } | Select-Object -Last 1))
        if ($at -ge 0) { $section = $lines[$at..($lines.Count - 1)] | Where-Object { $_.Trim() } }
    }
    $installers = @(Get-ChildItem (Join-Path $artifacts 'installer') -Filter *.exe -ErrorAction SilentlyContinue)
    $md = [Collections.Generic.List[string]]::new()
    $md.Add("# CI: $short $(if ($failed) { 'FAILED' } else { 'passed' })")
    $md.Add('')
    $md.Add("Commit ``$sha``: $subject  ")
    $md.Add(("Run {0:yyyy-MM-dd HH:mm}, {1:N1} min, on {2}" -f $start, ((Get-Date) - $start).TotalMinutes, $env:COMPUTERNAME))
    $md.Add('')
    $md.Add('| Step | Result |')
    $md.Add('|---|---|')
    $steps | ForEach-Object { $md.Add($_) }
    $md.Add('')
    if ($section.Count -gt 0) { $md.Add('```'); $section | ForEach-Object { $md.Add($_) }; $md.Add('```'); $md.Add('') }
    foreach ($i in $installers) { $md.Add(("Installer ``{0}`` SHA256 ``{1}``" -f $i.Name, (Get-FileHash $i.FullName -Algorithm SHA256).Hash)); $md.Add('') }
    if (Test-Path $testTable) { [IO.File]::ReadAllLines($testTable) | ForEach-Object { $md.Add($_) } }
    [IO.File]::WriteAllLines($summary, [string[]]$md, [Text.UTF8Encoding]::new($false))

    if (Test-Path $work) { Remove-Item -LiteralPath $work -Recurse -Force }
    Say ("ci: {0} for {1} in {2:N1} min; summary {3}" -f $(if ($failed) { 'FAILED' } else { 'PASSED' }), $short, ((Get-Date) - $start).TotalMinutes, $summary)
}
if ($failed) { exit 1 }
exit 0
