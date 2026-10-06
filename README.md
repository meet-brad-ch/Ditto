# Ditto, local-only fork

**What:** A fork of the Ditto clipboard manager (sabrogden/Ditto) that is built locally, with all
network code removed from the build.

**Why:** For safety. The owner has used Ditto for years. A clipboard history holds passwords and
private text, so the copy the owner runs is compiled from source they can read, and it has no
code that can send anything off the machine.

**Status:** partly working. All network code listed below is removed, the build is clean, and
`tools/verify.ps1` passes. The running app has not been checked yet: no runtime socket check, no
UI check.

What this fork removed (found by reading upstream a80fd35):

- **Friends.** Winsock TCP send and receive of clips, with the server on port 23443. The server
  starts by default on a normal install.
- **HTML clip preview.** It uses the IE WebBrowser control, which fetches remote content inside
  copied HTML.
- **Hand-offs to other programs.** URLs go to the browser through ShellExecute: Help, Web Search,
  Translate, Gmail, the QR URL and links. Email goes to the mail client through MAPI.

Upstream has no update check, telemetry or HTTP client. `httplib.h` and `sqlite/sqcloud.*` were
in the repo but were never compiled. They are deleted too.

Settings stay compatible. Action numbers are unchanged (`ActionEnums::Removed`), so saved
keyboard shortcuts still map to the same actions. Network settings left in an existing
ini or registry are ignored.

## How to run

These commands are verified 2026-10-06: a full Release|x64 rebuild in about 2 min with no errors.
Run them from the repo root in PowerShell. The simplest way is `tools\verify.ps1`, which does the
same and then runs the gates.

```
C:\vcpkg\vcpkg.exe install --triplet x64-windows-static-md --x-install-root=vcpkg_installed\x64-windows-static-md
$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msb CP_Main_10.sln /p:Configuration=Release /p:Platform=x64 /m
Release64\Ditto.exe
```

The build writes `Release64\Ditto.exe`, `Release64\Addins\DittoUtil.dll`, `ICU_Loader.dll` and
`focus.dll`.

**Third-party libraries come from vcpkg, not NuGet.**
- **Manifest:** `vcpkg.json` lists zlib 1.3.1 and gtest 1.17.0. It is pinned by its
  `builtin-baseline`, the vcpkg commit `0e39c107…`.
- **Wiring:** `Directory.Build.props` and `.targets` hook vcpkg into every project, using the
  `x64-windows-static-md` triplet (static libraries, dynamic CRT). Auto-link is off, so each
  project lists the libraries it links. A user-wide `vcpkg integrate install` is ignored.
- **Separate install step:** run `vcpkg install` first. Under `/m`, each project would otherwise
  start its own install, and the app compiled before zlib had finished installing.
- **SQLite:** SQLite3MultipleCiphers stays vendored in `src\sqlite`.

Baseline imports of upstream `Ditto.exe`: **WS2_32.dll** (Friends sockets) and **WININET.dll**
(`InternetCanonicalizeUrl`). This fork removes both.

**Verify:** `powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1` runs three
stages and exits 1 on the first failed one:

1. It installs the `vcpkg.json` dependencies, then builds Release|x64.
2. It runs `dumpbin /imports` on every `.exe`/`.dll` in `Release64`. No binary may import
   ws2_32, wsock32, mswsock, wininet, winhttp, urlmon, mapi32, dnsapi, iphlpapi, webio or
   httpapi.
3. It greps every source, `.rc` and `.vcxproj` file for socket, WinINet, WinHTTP, urlmon, MAPI and
   WebBrowser APIs, network DLL names and `ShellExecute` of an `http(s)://` URL.
4. It checks that `DittoSetup_10.iss` adds no firewall rules (netsh) and launches no URLs.

`-SkipBuild` skips stage 1. The gate was tested against faults planted on purpose: a seeded
`WSAStartup` line and a copied `curl.exe` were both caught.

**Prerequisites:**

- Visual Studio Community 2026 18.9, MSVC 14.51, toolset v145, with the C++ desktop workload.
- The MFC component (`Microsoft.VisualStudio.Component.VC.ATLMFC`). Install it with:
  `setup.exe modify --installPath "C:\Program Files\Microsoft Visual Studio\18\Community" --add Microsoft.VisualStudio.Component.VC.ATLMFC --passive`
  (run from `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer`, elevated).
- Windows SDK 10.0.26100.0.
- vcpkg at `C:\vcpkg`, or set `VCPKG_ROOT`. It must have the baseline commit from `vcpkg.json`
  (verified with vcpkg tool 2025-10-16).

## Installer

Inno Setup (open source) builds the installer from `DittoSetup\DittoSetup_10.iss`. These commands
were verified 2026-10-06 with Inno Setup 7.0.2. Build Release|x64 first, then run:

```
& "C:\Program Files\Inno Setup 7\ISCC.exe" DittoSetup\DittoSetup_10.iss
```

- **Output:** `DittoSetup\Output\DittoLocalSetup_<exe version>.exe`, unsigned. The version comes
  from `Ditto.exe`, which is currently 3.24.238.2 from `CP_Main.rc`.
- **What it packages:** Ditto.exe, ICU_Loader.dll, Addins\DittoUtil.dll, the VC++/MFC runtime
  DLLs from System32 (14.51), and `Debug\Language` and `Debug\Themes`.
- **What changed from upstream's script:**
  - The netsh firewall task (TCP 23443) is removed.
  - The post-install "View Help" and "View Change History" browser links are removed.
  - The publisher names the fork.
  - The runtime DLL folder works with both 32-bit and 64-bit ISCC.
  - `{pf}` is now `{commonpf}`.
- **Existing install:** the AppName is still "Ditto", so the installer upgrades an existing Ditto
  install in place and keeps its settings. It closes a running Ditto while it installs.
- **Warnings:** the remaining ISCC warnings come from upstream: outdated unofficial translations,
  unused variables, and HKCU writes from an admin install.

## Remotes

- `origin`: https://github.com/meet-brad-ch/Ditto (the fork)
- `upstream`: https://github.com/sabrogden/Ditto. This is a hard fork: no merges. To take an
  upstream fix, `git fetch upstream`, read the commit, and port it by hand into the fork's
  structure.

## Decisions

- 2026-10-06: Network code is deleted outright, not put behind `#ifdef`. With the code gone, a
  source grep and an import scan can prove there is none.
- 2026-10-06: The removal also covers browser and mail hand-offs and the WebBrowser HTML preview,
  not only sockets.
- 2026-10-06: Kept Ditto's MFC code and the upstream toolset (v145), and did not port to
  something else.
- 2026-10-06: Removed upstream's GitHub workflows, because they publish to Chocolatey,
  SignPath and GitHub Releases.
- 2026-10-06: Kept the removed action enum values and marked them `Removed`, instead of
  deleting them. Shortcuts are saved as `QP_ShortCut_<number>_…`, so renumbering would remap
  existing shortcuts.
- 2026-10-06: HTML clips show text or RTF, not a rendered page. The only renderer available was
  the IE WebBrowser control, which loads remote content.
- 2026-10-06: Third-party libraries come from a vcpkg manifest instead of NuGet `packages.config`.
  zlib went from 1.2.11 (2017) to 1.3.1. The unused libpng package and the orphan
  `src\zlib\*.h` and `src\sqlite\lz4.*` files are gone.
- 2026-10-06: Owner chose a hard fork: no upstream merges, upstream fixes are cherry-picked by
  hand. The ranked update plan is: safety fixes first, then test foundation, clipboard-core
  refactor, toolchain, libraries.
- 2026-10-06: Kept Inno Setup for the installer: it is open source and was already upstream's
  tool. Only the firewall and browser-link parts of the script were removed.
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

