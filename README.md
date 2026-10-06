# Ditto, local-only fork

**What:** A fork of the Ditto clipboard manager (sabrogden/Ditto) that is built locally, with all
network code removed from the build.

**Why:** For safety. The owner has used Ditto for years. A clipboard history holds passwords and
private text, so the copy the owner runs is compiled from source they can read, and it has no
code that can send anything off the machine.

**Status:** scaffold only. This is upstream master a80fd35 with the fork files added. No network
code has been removed yet, and no local build has been done yet.

What "network code" means in this fork, found by reading upstream a80fd35:

- **Friends.** Winsock TCP send and receive of clips, with the server on port 23443. The server
  starts by default on a normal install.
- **HTML clip preview.** It uses the IE WebBrowser control, which fetches remote content inside
  copied HTML.
- **Hand-offs to other programs.** URLs go to the browser through ShellExecute: Help, Web Search,
  Translate, Gmail, the QR URL and links. Email goes to the mail client through MAPI.

Upstream has no update check, telemetry or HTTP client. `httplib.h` and `sqlite/sqcloud.*` are
in the repo but are never compiled.

## How to run

Not yet run. The commands below are planned and get replaced by the verified ones after the
first build.

```
msbuild CP_Main_10.sln /t:restore /p:RestorePackagesConfig=true
msbuild CP_Main_10.sln /p:Configuration=Release /p:Platform=x64
Release64\Ditto.exe
```

**Verify:** none yet. `tools/verify.ps1` is planned: it builds, scans the binary imports and
greps the source.

**Prerequisites:**

- Visual Studio 2026 (18.x) with the C++ desktop workload and toolset v145.
- The MFC component (`Microsoft.VisualStudio.Component.VC.ATLMFC`).
- Windows SDK 10.0.26100.0.

## Remotes

- `origin`: https://github.com/meet-brad-ch/Ditto (the fork)
- `upstream`: https://github.com/sabrogden/Ditto. To merge upstream:
  `git fetch upstream; git merge upstream/master`. If upstream changed a file this fork deleted,
  keep the delete.

## Decisions

- 2026-10-06: Network code is deleted outright, not put behind `#ifdef`. With the code gone, a
  source grep and an import scan can prove there is none.
- 2026-10-06: The removal also covers browser and mail hand-offs and the WebBrowser HTML preview,
  not only sockets.
- 2026-10-06: Kept Ditto's MFC code and the upstream toolset (v145), and did not port to
  something else.
- 2026-10-06: Removed upstream's GitHub workflows, because they publish to Chocolatey,
  SignPath and GitHub Releases.
- 2026-10-06: Renamed `ReadMe.md` to `README.md`. The two names collide on Windows. The upstream
  readme text is kept below.

---

Below: the upstream readme, unchanged.

# [Ditto - Clipboard Manager](https://github.com/sabrogden/Ditto/releases/download/3.25.113.0/DittoSetup_3_25_113_0.exe)

![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/sabrogden/Ditto/latest/total) ![GitHub commits since latest release](https://img.shields.io/github/commits-since/sabrogden/Ditto/latest) ![GitHub contributors](https://img.shields.io/github/contributors/sabrogden/Ditto)



[Help/Wiki](https://github.com/sabrogden/Ditto/wiki)&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[Forums](https://github.com/sabrogden/Ditto/issues)&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[Donate](https://www.paypal.com/donate/?item_name=Donation+to+Ditto&cmd=_donations&business=sabrogden%40gmail.com&Z3JncnB0=)&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[Beta](https://ditto-cp.sourceforge.io/beta/)&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[Source](https://github.com/sabrogden/Ditto)&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[History](https://github.com/sabrogden/Ditto/releases)

Ditto is an extension to the standard windows clipboard. It saves each item placed on the clipboard allowing you access to any of those items at a later time. Ditto allows you to save any type of information that can be put on the clipboard, text, images, html, custom formats.


1. [Installer](https://github.com/sabrogden/Ditto/releases/download/3.25.113.0/DittoSetup_3_25_113_0.exe)
2. [Portable](https://github.com/sabrogden/Ditto/releases/download/3.25.113.0/DittoPortable_3_25_113_0.zip)
3. [Chocolatey](https://chocolatey.org/packages/ditto/3.23.124.0) choco install ditto
4. [Chocolatey Portable](https://chocolatey.org/packages/ditto.portable/3.23.124.0) choco install ditto.portable
5. [Winget](https://winget.run/pkg/Ditto/Ditto) winget install -e --id Ditto.Ditto
6. [Windows Store App](https://www.microsoft.com/en-us/store/p/ditto-cp/9nblggh3zbjq)  


## Basic Usage

1. Run Ditto
2. Copy things to the clipboard, e.g. using Ctrl-C with text selected in a text editor.
3. Open Ditto by clicking its icon in the system tray or by pressing its Hot Key which defaults to Ctrl + ` – i.e. hold down Ctrl and press the back-quote (tilde ~) key.
4. Double click or press enter on the item to paste it to the previous window.

## Local First
- No login
- No cloud
- No telemetry

## Windows Code-Signing Policy
Free code signing on Windows binaries provided by SignPath.io, certificate by SignPath Foundation.
<br>
<br>

<img src="ditto.gif">

