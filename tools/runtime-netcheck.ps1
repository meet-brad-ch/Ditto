# Runtime network check for the fork without network code. Runs the built Ditto as a portable copy, puts four
# kinds of clipboard content on the clipboard (text, CF_HTML with a remote <img>, an image, a file
# list), and watches the TCP/UDP endpoints the Ditto process owns. Any endpoint is a finding.
# Prints one timestamped line per step and FAILED lines on findings; exits 1 on any finding.
# The portable copy is stopped and its folder (database, log) deleted at the end, so nothing copied
# while it ran is kept. The clipboard text from before the run is put back.
# Usage (repo root, after a build):
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\runtime-netcheck.ps1 [-WatchSeconds 20]
param(
    [string] $BuildDir = (Join-Path (Split-Path -Parent $PSScriptRoot) 'Release64'),
    [int] $WatchSeconds = 20
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
function Say([string] $m) { Write-Output "$(Get-Date -Format 'HH:mm:ss') $m" }

$script:findings = 0
function Test-Sockets([int] $procId, [string] $when) {
    $tcp = @(Get-NetTCPConnection | Where-Object { $_.OwningProcess -eq $procId })
    $udp = @(Get-NetUDPEndpoint | Where-Object { $_.OwningProcess -eq $procId })
    $tcp | ForEach-Object { Say "FAILED $when TCP $($_.LocalAddress):$($_.LocalPort) -> $($_.RemoteAddress):$($_.RemotePort) $($_.State)" }
    $udp | ForEach-Object { Say "FAILED $when UDP $($_.LocalAddress):$($_.LocalPort)" }
    $script:findings += $tcp.Count + $udp.Count
}

$exe = Join-Path $BuildDir 'Ditto.exe'
if (-not (Test-Path $exe)) { Say "FAILED no Ditto.exe in $BuildDir (build Release|x64 first)"; exit 1 }
$runDir = Join-Path ([IO.Path]::GetTempPath()) ("ditto-netcheck-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory $runDir | Out-Null
Copy-Item (Join-Path $BuildDir '*') $runDir -Recurse -Include *.exe, *.dll
New-Item -ItemType File (Join-Path $runDir 'portable') | Out-Null
Say "copied build to $runDir (portable)"

$savedText = [Windows.Forms.Clipboard]::GetText()
Say "saved current clipboard text ($($savedText.Length) chars)"
$p = $null
try {
    $p = Start-Process (Join-Path $runDir 'Ditto.exe') -WorkingDirectory $runDir -PassThru
    Say "started Ditto.exe pid $($p.Id)"
    Start-Sleep -Seconds 4
    if ($p.HasExited) { Say "FAILED Ditto exited at startup, code $($p.ExitCode)"; exit 1 }
    Test-Sockets $p.Id 'after start:'

    [Windows.Forms.Clipboard]::SetText("ditto-netcheck: plain text $(Get-Date -Format o)")
    Say 'clipboard: text'; Start-Sleep -Seconds 2; Test-Sockets $p.Id 'after text:'

    # CF_HTML whose fragment loads a remote image: a renderer would fetch it
    $frag = '<p>ditto-netcheck html</p><img src="http://example.com/ditto-netcheck.png">'
    $html = "<html><body><!--StartFragment-->$frag<!--EndFragment--></body></html>"
    $hdr = "Version:0.9`r`nStartHTML:{0:D10}`r`nEndHTML:{1:D10}`r`nStartFragment:{2:D10}`r`nEndFragment:{3:D10}`r`n"
    $hdrLen = ($hdr -f 0, 0, 0, 0).Length
    $utf8 = [Text.Encoding]::UTF8
    $startFrag = $hdrLen + $utf8.GetByteCount($html.Substring(0, $html.IndexOf('<!--StartFragment-->') + 20))
    $endFrag = $hdrLen + $utf8.GetByteCount($html.Substring(0, $html.IndexOf('<!--EndFragment-->')))
    $cfHtml = ($hdr -f $hdrLen, ($hdrLen + $utf8.GetByteCount($html)), $startFrag, $endFrag) + $html
    $data = New-Object Windows.Forms.DataObject
    $data.SetData([Windows.Forms.DataFormats]::Html, (New-Object IO.MemoryStream (, $utf8.GetBytes($cfHtml))))
    $data.SetData([Windows.Forms.DataFormats]::UnicodeText, 'ditto-netcheck html')
    [Windows.Forms.Clipboard]::SetDataObject($data, $true)
    Say 'clipboard: HTML with remote <img>'; Start-Sleep -Seconds 2; Test-Sockets $p.Id 'after html:'

    $bmp = New-Object Drawing.Bitmap 64, 64
    [Drawing.Graphics]::FromImage($bmp).Clear([Drawing.Color]::SteelBlue)
    [Windows.Forms.Clipboard]::SetImage($bmp)
    Say 'clipboard: image 64x64'; Start-Sleep -Seconds 2; Test-Sockets $p.Id 'after image:'

    $file = Join-Path $runDir 'ditto-netcheck.txt'
    Set-Content $file 'ditto-netcheck file'
    $files = New-Object Collections.Specialized.StringCollection
    [void]$files.Add($file)
    [Windows.Forms.Clipboard]::SetFileDropList($files)
    Say 'clipboard: file list'; Start-Sleep -Seconds 2; Test-Sockets $p.Id 'after files:'

    Say "watching sockets for $WatchSeconds s"
    for ($i = 1; $i -le $WatchSeconds; $i++) { Start-Sleep -Seconds 1; Test-Sockets $p.Id "watch $i s:" }

    $db = Get-ChildItem $runDir -Filter *.db | Select-Object -First 1
    if ($db) { Say "db: $($db.Name) $($db.Length) bytes, modified $($db.LastWriteTime.ToString('HH:mm:ss'))" }
    else { Say 'FAILED no database file created'; $script:findings++ }

    $mods = @($p.Modules | ForEach-Object { $_.ModuleName.ToLowerInvariant() })
    $netMods = @($mods | Where-Object { $_ -match '^(ws2_32|wsock32|mswsock|wininet|winhttp|urlmon|mapi32|mshtml|ieframe|dnsapi)\.dll$' })
    Say "loaded modules: $($mods.Count); network-related ones loaded by Windows components: $(if ($netMods) { $netMods -join ', ' } else { 'none' })"
}
finally {
    if ($p -and -not $p.HasExited) {
        Stop-Process -Id $p.Id -Force
        if (-not $p.WaitForExit(10000)) { Say "FAILED Ditto pid $($p.Id) did not stop; $runDir left in place"; exit 1 }
        Say "stopped Ditto pid $($p.Id)"
    }
    Remove-Item $runDir -Recurse -Force
    Say "deleted $runDir"
    if ($savedText) { [Windows.Forms.Clipboard]::SetText($savedText); Say 'restored the previous clipboard text' }
}

if ($script:findings -gt 0) { Say "FAILED $($script:findings) socket/db findings"; exit 1 }
Say 'RUNTIME OK: no TCP/UDP endpoints owned by Ditto during start, 4 clip types and the watch'
exit 0
