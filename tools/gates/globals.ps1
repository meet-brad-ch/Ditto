# Globals gate: Ditto's own code is object-oriented (owner rule): behaviour lives in classes, and
# there is no global mutable state. Found by a light C++ scan (braces classified as namespace,
# class or code block):
#   free-function     a function defined outside a class (unqualified, or qualified by namespaces only)
#   global-variable   a variable at namespace scope (constants included: they belong in a class)
#   static-member     a non-constant static data member
#   local-static      a non-constant static variable inside a function
#   macro             a #define (owner 2026-10-07: avoid macros; include guards do not count)
#  - contract code (lib\): none;
#  - legacy code: each finding (file, kind, name) is held in tools\baselines\globals.tsv by
#    tools\ratchet.ps1, so none may be added (Phase L3 takes it to the allow-list);
#  - tools\gates\globals-allow.txt: the accepted exceptions (owner 2026-10-07), each with a reason.
# Untouched third-party code (tools\thirdparty.txt) and tests\ (gtest's TEST functions) are skipped.
param(
    [Parameter(Mandatory)] [string] $Repo,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }
. (Join-Path $PSScriptRoot 'sources.ps1')

# The code of a file without comments, string/character literal text and preprocessor lines
function Get-Code([string] $path) {
    $text = [IO.File]::ReadAllText($path)
    $text = [regex]::Replace($text, '(?s)/\*.*?\*/', ' ')
    $text = [regex]::Replace($text, '//[^\r\n]*', '')
    $text = [regex]::Replace($text, 'R"([^(\s]*)\((?s:.*?)\)\1"', '""')
    $text = [regex]::Replace($text, '"(\\.|[^"\\\r\n])*"', '""')
    $text = [regex]::Replace($text, "'(\\.|[^'\\\r\n])*'", "''")
    # preprocessor lines, with their backslash continuations
    $text = [regex]::Replace($text, '(?m)^[ \t]*#(?:[^\r\n]*\\\r?\n)*[^\r\n]*', '')
    # invocations of all-caps macros (MFC's BEGIN_MESSAGE_MAP(...), IMPLEMENT_DYNAMIC(...), ...):
    # they are not functions or variables of Ditto's; the macros Ditto defines are counted apart
    $text = [regex]::Replace($text, '\b[A-Z_][A-Z0-9_]{2,}\s*\((?>[^()]+|\((?<d>)|\)(?<-d>))*(?(d)(?!))\)', ' ')
    return $text
}

# The macros a file defines: every #define except an include guard (#ifndef X / #define X)
function Find-Macros([string] $path) {
    $text = [IO.File]::ReadAllText($path)
    $text = [regex]::Replace($text, '(?s)/\*.*?\*/', ' ')
    $guards = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($m in [regex]::Matches($text, '(?m)^[ \t]*#[ \t]*if(?:ndef[ \t]+|[ \t]+!\s*defined\s*\(\s*)([A-Za-z_]\w*)')) { [void]$guards.Add($m.Groups[1].Value) }
    $findings = [Collections.Generic.List[string]]::new()
    foreach ($m in [regex]::Matches($text, '(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)([^\r\n]*)')) {
        $name = $m.Groups[1].Value
        if ($guards.Contains($name) -and -not $m.Groups[2].Value.Trim()) { continue }
        $findings.Add("macro`t$name")
    }
    return , $findings
}

# Names of the namespaces declared in the own sources, to tell ns::Function from Class::Method
function Get-Namespaces([string[]] $codes) {
    $set = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($code in $codes) { foreach ($m in [regex]::Matches($code, '\bnamespace\s+([A-Za-z_]\w*)')) { [void]$set.Add($m.Groups[1].Value) } }
    foreach ($n in 'std', 'ATL', 'Gdiplus', 'DittoCore', 'tinyxml2', 'qrcodegen') { [void]$set.Add($n) }
    return , $set
}

$declSkip = '^\s*(using|typedef|friend|template|static_assert|extern|class|struct|union|enum|namespace|public|private|protected|return)\b'
$macroCall = '^\s*[A-Z_][A-Z0-9_]*\s*\('

# The declared name of a variable statement: the last identifier before '=', '{', '[' or the end
function Get-DeclaredName([string] $statement) {
    $head = ($statement -split '[={\[]', 2)[0]
    $m = [regex]::Match($head, '([A-Za-z_][\w:]*)\s*$')
    return $m.Groups[1].Value
}

# Is a function header (text before its body) a free function? Returns its name, or '' for a member.
function Get-FreeFunctionName([string] $header, $namespaces) {
    $header = $header -replace '__declspec\s*\([^)]*\)', ''   # an attribute, not the function's name
    $m = [regex]::Match($header, '((?:[A-Za-z_]\w*\s*::\s*)*~?(?:operator\s*[^\s(]+|[A-Za-z_]\w*))\s*\([^;]*$')
    if (-not $m.Success) { return '' }
    $name = $m.Groups[1].Value -replace '\s', ''
    $qualifiers = @($name -split '::' | Select-Object -SkipLast 1)
    foreach ($q in $qualifiers) { if (-not $namespaces.Contains($q)) { return '' } }   # Class::Method
    if ($name -match '^(if|for|while|switch|catch|sizeof|decltype|return)$') { return '' }
    return $name
}

function Find-Globals([string] $code, $namespaces) {
    $findings = [Collections.Generic.List[string]]::new()
    $stack = [Collections.Generic.List[string]]::new()   # 'ns', 'class', 'typedef-class', 'body' (function), 'init'
    $buffer = [Text.StringBuilder]::new()
    $typedefNames = $false   # after 'typedef struct {...}': the names up to ';' are type names
    foreach ($ch in $code.ToCharArray()) {
        if ($ch -eq '{') {
            $header = $buffer.ToString()
            $atNamespace = -not ($stack | Where-Object { $_ -ne 'ns' })
            if ($header -match '\bnamespace\b[^;]*$' -or $header -match 'extern\s*""\s*$') { $kind = 'ns' }
            elseif ($header -match '\b(class|struct|union|enum)\b[^;()=]*$') { $kind = $(if ($header -match '^\s*typedef\b') { 'typedef-class' } else { 'class' }) }
            elseif ($header -match '\)\s*(const|noexcept|override|final|mutable|->[^{]*|:[^{]*|\s)*$' -or $header -match '\b(do|else|try)\s*$') { $kind = 'body' }
            else { $kind = 'init' }
            if ($kind -eq 'body' -and $atNamespace) {
                $name = Get-FreeFunctionName $header $namespaces
                if ($name) { $findings.Add("free-function`t$name") }
            }
            $stack.Add($kind)
            if ($kind -ne 'init') { [void]$buffer.Clear() } else { [void]$buffer.Append('{') }
            continue
        }
        if ($ch -eq '}') {
            if ($stack.Count -gt 0) {
                $kind = $stack[$stack.Count - 1]; $stack.RemoveAt($stack.Count - 1)
                if ($kind -eq 'init') { [void]$buffer.Append('}'); continue }
                if ($kind -eq 'typedef-class') { $typedefNames = $true }
            }
            [void]$buffer.Clear()
            continue
        }
        if ($ch -eq ';') {
            $statement = ($buffer.ToString() -replace '\s+', ' ').Trim()
            [void]$buffer.Clear()
            if ($typedefNames) { $typedefNames = $false; continue }
            if (-not $statement -or $statement -match $declSkip -or $statement -match $macroCall) { continue }
            $scope = if ($stack.Count -eq 0) { 'ns' } else { $stack[$stack.Count - 1] }
            if ($stack | Where-Object { $_ -eq 'init' }) { continue }
            $isConst = $statement -match '\b(const|constexpr|constinit)\b'
            if ($scope -eq 'ns' -and -not ($stack | Where-Object { $_ -ne 'ns' })) {
                # a function declaration has '(' before any '=' and no initializer braces
                $beforeAssign = ($statement -split '=', 2)[0]
                if ($beforeAssign -match '\(' -and $beforeAssign -notmatch '\(\s*\*') { continue }
                $name = Get-DeclaredName $statement
                if (-not $name -or $name -match '::') { continue }   # Class::m_static definition: counted in the class
                $findings.Add("global-variable`t$name")
            }
            elseif ($scope -in 'class', 'typedef-class') {
                if ($statement -match '\bstatic\b' -and -not $isConst -and ($statement -split '=', 2)[0] -notmatch '\(') {
                    $findings.Add("static-member`t$(Get-DeclaredName $statement)")
                }
            }
            elseif ($statement -match '^\s*static\b' -and -not $isConst -and ($statement -split '=', 2)[0] -notmatch '\(') {
                $findings.Add("local-static`t$(Get-DeclaredName $statement)")
            }
            continue
        }
        [void]$buffer.Append($ch)
    }
    return , $findings
}

# accepted exceptions: path<TAB>kind<TAB>name<TAB>reason
$allowed = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($line in [IO.File]::ReadAllLines((Join-Path $PSScriptRoot 'globals-allow.txt'))) {
    if (-not $line.Trim() -or $line.TrimStart().StartsWith('#')) { continue }
    $parts = $line.Split("`t")
    if ($parts.Count -lt 4 -or -not $parts[3].Trim()) { throw "globals-allow.txt: an exception needs path, kind, name and a reason: '$line'" }
    [void]$allowed.Add("$($parts[0])`t$($parts[1])`t$($parts[2])")
}

$files = @(Get-OwnSources $Repo | Where-Object { $_ -notmatch '^tests\\' })
$codes = [ordered]@{}
foreach ($rel in $files) { $codes[$rel] = Get-Code (Join-Path $Repo $rel) }
$namespaces = Get-Namespaces @($codes.Values)

$contractFindings = 0
$counts = [Collections.Generic.Dictionary[string, int]]::new([StringComparer]::Ordinal)
$allowedHits = 0
foreach ($rel in $codes.Keys) {
    $fileFindings = [Collections.Generic.List[string]]::new()
    $fileFindings.AddRange((Find-Globals $codes[$rel] $namespaces))
    # resource.h: the resource compiler's symbol IDs, which must be macros
    if ((Split-Path $rel -Leaf) -ne 'resource.h') { $fileFindings.AddRange((Find-Macros (Join-Path $Repo $rel))) }
    foreach ($finding in $fileFindings) {
        $key = "$rel`t$finding"
        if ($allowed.Contains($key)) { $allowedHits++; continue }
        if ($rel -match '^lib\\') { Say ("globals: FAIL contract code {0}: {1}" -f $rel, ($finding -replace "`t", ' ')); $contractFindings++; continue }
        $key = $key.ToLowerInvariant()
        $counts[$key] = $(if ($counts.ContainsKey($key)) { $counts[$key] } else { 0 }) + 1
    }
}
$total = 0; foreach ($v in $counts.Values) { $total += $v }
Say ("globals: {0} files; contract code {1}; legacy code {2} findings; {3} accepted exceptions" -f $files.Count, $(if ($contractFindings) { "$contractFindings FAILED" } else { 'none' }), $total, $allowedHits)
if ($contractFindings -gt 0) { Say "globals: FAILED $contractFindings globals in lib\"; exit 1 }

$outDir = Join-Path $Repo 'build\logs'
New-Item -ItemType Directory $outDir -Force | Out-Null
$current = Join-Path $outDir 'globals.tsv'
$lines = @($counts.Keys | Sort-Object -CaseSensitive | ForEach-Object { "$_`t$($counts[$_])" })
[IO.File]::WriteAllLines($current, [string[]]$lines, [Text.UTF8Encoding]::new($false))
$ratchetArgs = @{ Name = 'globals'; Current = $current; Baseline = (Join-Path $Repo 'tools\baselines\globals.tsv') }
if ($Update) { $ratchetArgs['Update'] = $true }
& (Join-Path $Repo 'tools\ratchet.ps1') @ratchetArgs
exit $LASTEXITCODE
