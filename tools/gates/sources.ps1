# Dot-sourced by the gates: the C/C++ files that are Ditto's own code.
# Get-OwnSources -Repo <root>  -> repo-relative paths (backslashes), sorted.
# Own code = every tracked .c/.cpp/.h/.hpp in the source folders (git ls-files: generated and
# ignored files such as the MIDL output and build folders never count, and a local run sees what a
# clean clone sees), minus the untouched third-party files in tools\thirdparty.txt.

function Get-ThirdPartyFiles([string] $Repo) {
    $set = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($line in [IO.File]::ReadAllLines((Join-Path $Repo 'tools\thirdparty.txt'))) {
        $entry = $line.Trim()
        if ($entry -and -not $entry.StartsWith('#')) {
            if (-not (Test-Path (Join-Path $Repo $entry))) { throw "tools\thirdparty.txt lists a missing file: $entry" }
            [void]$set.Add($entry)
        }
    }
    return , $set
}

function Get-OwnSources([string] $Repo) {
    $thirdParty = Get-ThirdPartyFiles $Repo
    $dirs = 'src', 'Shared', 'Addins', 'ICU_Loader', 'focusdll', 'FocusHighlight', 'lib', 'tests'
    $tracked = & git -C $Repo ls-files -- $dirs
    if ($LASTEXITCODE -ne 0) { throw "git ls-files failed in $Repo" }
    $files = $tracked | ForEach-Object { $_ -replace '/', '\' } |
        Where-Object { $_ -match '\.(c|cpp|h|hpp)$' -and -not $thirdParty.Contains($_) }
    return @($files | Sort-Object)
}
