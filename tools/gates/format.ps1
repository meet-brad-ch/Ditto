# Formatting gate: every C/C++ file of Ditto's own code is formatted as .clang-format says
# (owner decision 2026-10-07: clang-format on all own code). Untouched third-party files
# (tools\thirdparty.txt) and the resource editor's resource.h files are not formatted.
# clang-format is the one Visual Studio ships (VC\Tools\Llvm\x64\bin); its version is printed,
# because another version may format differently.
#   -Fix   formats the files in place instead of checking them
param(
    [Parameter(Mandatory)] [string] $Repo,
    [switch] $Fix
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
. (Join-Path $PSScriptRoot 'sources.ps1')

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { Say "format: FAILED vswhere.exe not found at $vswhere"; exit 1 }
$vs = & $vswhere -latest -products * -property installationPath
$clangFormat = Join-Path $vs 'VC\Tools\Llvm\x64\bin\clang-format.exe'
if (-not (Test-Path $clangFormat)) { Say "format: FAILED clang-format not found at $clangFormat"; exit 1 }
$version = (& $clangFormat --version | Select-Object -First 1)
Say "format: $version"

# resource.h is the resource editor's file (some are UTF-16, which clang-format cannot read)
$files = @(Get-OwnSources $Repo | Where-Object { (Split-Path $_ -Leaf) -ne 'resource.h' })
$style = "--style=file:$(Join-Path $Repo '.clang-format')"
$unformatted = 0
Push-Location $Repo
try {
    foreach ($rel in $files) {
        if ($Fix) {
            & $clangFormat $style -i $rel
            if ($LASTEXITCODE -ne 0) { Say "format: FAILED clang-format exit $LASTEXITCODE on $rel"; exit 1 }
            continue
        }
        # Windows PowerShell 5.1 turns a native program's stderr into error records, which
        # 'Stop' would throw on: read clang-format's report as plain text instead
        $ErrorActionPreference = 'Continue'
        $out = @(& $clangFormat $style --dry-run --Werror $rel 2>&1 | ForEach-Object { "$_" })
        $code = $LASTEXITCODE
        $ErrorActionPreference = 'Stop'
        if ($code -ne 0) {
            $unformatted++
            $first = ($out | Where-Object { "$_" -match ':\d+:\d+:' } | Select-Object -First 1)
            Say "format: FAIL $rel ($first)"
        }
    }
}
finally { Pop-Location }

if ($Fix) { Say "format: formatted $($files.Count) files"; exit 0 }
Say ("format: {0} files; {1} not formatted" -f $files.Count, $unformatted)
if ($unformatted -gt 0) { Say "format: FAILED $unformatted files differ from .clang-format (fix: tools\gates\format.ps1 -Repo . -Fix)"; exit 1 }
Say 'format: ok   every file matches .clang-format'
exit 0
