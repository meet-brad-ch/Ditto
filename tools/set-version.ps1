# Stamps one version into CP_Main.rc: FILEVERSION and PRODUCTVERSION (a,b,c,d) and the string
# values "FileVersion" and "ProductVersion" ("a.b.c.d"). Ditto.exe, its About box
# (src\AppVersion.cpp reads the version resource) and the installer's file name
# (DittoSetup_a_b_c_d.exe: the .iss takes the version from Ditto.exe) all follow it.
# CI runs this on a tag v<a>.<b>.<c>[.<d>] before the build; locally it is the same command.
# Exactly 2 + 2 places must match, otherwise it fails and writes nothing. Line endings are kept.
# Running it twice with the same version changes nothing.
# Usage (repo root):  powershell -NoProfile -ExecutionPolicy Bypass -File tools\set-version.ps1 -Version 3.25.0[.0]
param(
    [Parameter(Mandatory)] [string] $Version,
    [string] $Path = ''
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
# the default is set here: Windows PowerShell 5.1 leaves $PSScriptRoot empty in param() defaults
if (-not $Path) { $Path = Join-Path (Split-Path -Parent $PSScriptRoot) 'CP_Main.rc' }

if ($Version -notmatch '^(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?$') { Say "set-version: FAILED '$Version' is not <major>.<minor>.<patch>[.<build>]"; exit 1 }
$parts = @([int]$Matches[1], [int]$Matches[2], [int]$Matches[3], [int]$(if ($Matches[4]) { $Matches[4] } else { 0 }))
if (@($parts | Where-Object { $_ -gt 65535 }).Count -gt 0) { Say 'set-version: FAILED every part must be 0..65535 (the VERSIONINFO limit)'; exit 1 }
$dotted = $parts -join '.'
$comma = $parts -join ','

if (-not (Test-Path -LiteralPath $Path)) { Say "set-version: FAILED $Path not found"; exit 1 }
$text = [IO.File]::ReadAllText($Path)   # the file's own line endings stay as they are

$numeric = [regex]'(?m)^(\s*(?:FILE|PRODUCT)VERSION\s+)\d+,\d+,\d+,\d+(?=\s*$)'
$string = [regex]'(?m)^(\s*VALUE\s+"(?:FileVersion|ProductVersion)",\s*")\d+\.\d+\.\d+\.\d+(?=")'
$numericCount = $numeric.Matches($text).Count
$stringCount = $string.Matches($text).Count
if ($numericCount -ne 2 -or $stringCount -ne 2) {
    Say "set-version: FAILED $Path must have 2 FILEVERSION/PRODUCTVERSION lines and 2 FileVersion/ProductVersion values; found $numericCount and $stringCount"
    exit 1
}

$before = (($numeric.Match($text).Value.Trim() -split '\s+')[-1]) -replace ',', '.'
$new = $string.Replace($numeric.Replace($text, "`${1}$comma"), "`${1}$dotted")
if ($new -eq $text) { Say "set-version: $Path is already at $dotted"; exit 0 }
[IO.File]::WriteAllText($Path, $new, [Text.UTF8Encoding]::new($false))   # ASCII content: the same bytes as the original encoding
Say "set-version: $Path $before -> $dotted"
exit 0
