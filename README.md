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

**Verify:** `powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1` runs eight
stages and exits 1 on the first failed one:

1. It installs the `vcpkg.json` dependencies, then builds Release|x64.
2. It runs `dumpbin /imports` on every `.exe`/`.dll` in `Release64`. No binary may import
   ws2_32, wsock32, mswsock, wininet, winhttp, urlmon, mapi32, dnsapi, iphlpapi, webio or
   httpapi.
3. It runs `dumpbin /headers` on the same binaries. Each must carry high-entropy ASLR, dynamic
   base, DEP (NX) and Control Flow Guard.
4. It greps every source, `.rc` and `.vcxproj` file for socket, WinINet, WinHTTP, urlmon, MAPI and
   WebBrowser APIs, network DLL names and `ShellExecute` of an `http(s)://` URL.
5. It checks every Inno Setup script (`*.iss`): no firewall rules (netsh), no URL launches, and a
   `MinVersion` of Windows 10 or later.
6. It rejects raw allocation (`new`, `delete`, `malloc`, `free`) in `lib\` and `tests\`.
7. It runs every GoogleTest in `tests\` on its own (`--gtest_filter`), under AddressSanitizer.
8. It runs Doxygen (`tools\Doxyfile.contract`): every class, function and member of the contract
   code must be documented, and any Doxygen warning fails. The files are listed by name in that
   Doxyfile.

`-SkipBuild` skips stage 1. The gate was tested against faults planted on purpose: a seeded
`WSAStartup` line and a copied `curl.exe` were both caught. The hardening stage failed on the
binaries built before Control Flow Guard was on, and the installer stage on the old ARM64 script.

**Prerequisites:**

- Visual Studio Community 2026 18.9, MSVC 14.51, toolset v145, with the C++ desktop workload.
- The MFC component (`Microsoft.VisualStudio.Component.VC.ATLMFC`). Install it with:
  `setup.exe modify --installPath "C:\Program Files\Microsoft Visual Studio\18\Community" --add Microsoft.VisualStudio.Component.VC.ATLMFC --passive`
  (run from `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer`, elevated).
- Windows SDK 10.0.26100.0.
- vcpkg at `C:\vcpkg`, or set `VCPKG_ROOT`. It must have the baseline commit from `vcpkg.json`
  (verified with vcpkg tool 2025-10-16).

## Layout

- `src\`: the Ditto app (MFC), mostly upstream code.
- `lib\DittoCore\`: clipboard-format logic in plain C++, written to the contract. It has no MFC, no
  UI and no global state. Built at `/W4 /WX`, with `/sdl`, `/permissive-` and Control Flow Guard
  (`lib\Contract.props`).
- `tests\`: GoogleTest unit tests for DittoCore. The Release build uses AddressSanitizer; the
  output is `build\DittoTests\x64\Release\DittoTests.exe`.
- `tools\verify.ps1`: the quality gate.
- `DittoSetup\`: the Inno Setup installer.

**Rules for new and refactored code (owner, 2026-10-06):**
- **Fail fast.** An operation that cannot continue correctly stops at its top and shows the cause.
  There are no fallbacks and no swallowed errors.
- **Object-oriented.** Behavior lives in classes, with one class per `.h`/`.cpp` pair. Tests are
  exempt from strict OO.
- **No raw allocation.** Use `std::unique_ptr` and standard containers, and RAII for Win32
  resources.
- **Explicit project files.** Every file is listed by name; no wildcards.
- **Always initialize.** Every variable and member gets a brace initializer (`{}`).
- **Doxygen.** Every class, function and member is documented with `@brief`, `@param`, `@return`
  and `@throws`.
- **No dead code.** Unused code is deleted, not kept.

**Supported OS:** Windows 10 and 11 only (owner, 2026-10-06), from Windows 10 1607 (build 14393),
because Ditto calls `GetDpiForWindow` directly. Every project builds with `_WIN32_WINNT` and
`WINVER` at `0x0A00`, `/sdl` and Control Flow Guard, set in `Directory.Build.targets` after each
project's own settings, so no project can turn them off.

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
  - `MinVersion` is Windows 10 1607 (was Windows 7). The Windows 7/8 `cmd.exe` paste strings are
    no longer written; the leftover values are deleted on every install.
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
- 2026-10-06: CF_HDROP file lists are read by `DittoCore::FileDropList` and `GlobalFileDrop`, not
  `DragQueryFile` with fixed buffers. Upstream passed `sizeof` (bytes) as a character count at
  seven sites, so a clipboard path over 260 characters overflowed a stack buffer.
- 2026-10-06: Malformed clipboard data throws `DittoCore::ClipboardFormatError`. Only the top of
  each operation catches it, and it rejects that operation visibly: an error balloon through
  `CErrorReport`, the paste/drag popup, or an add-in message box.
  - The boundaries are copy, save clipboard, paste/drag, the OLE delayed-render callback (it
    returns FALSE, so nothing crosses COM), text-only paste (parsed before the clipboard is
    opened) and file-contents import.
- 2026-10-06: Clipboard text is read by `DittoCore::ClipText`, in the four aggregators (text,
  Unicode, HTML, RTF).
  - Upstream read `data[size-1]`, one byte before the buffer when a clip was empty. The HTML
    aggregator also wrote there. The RTF aggregator stepped 6 characters into clips of any length.
  - CF_TEXT and CF_UNICODETEXT must contain a terminating null.
  - CF_HTML and RTF are read up to the first null or the end of the block, because those formats
    are length-delimited.
  - A malformed clip now stops the paste with a message instead of being dropped silently.
- 2026-10-06: Fixed every other byte-vs-character count found by `/analyze`.
  - Six file dialogs passed `sizeof` as `nMaxFile`.
  - SendKeys checked a key name against `sizeof(KeyString)` and read window titles the same way.
  - The INI font name and an add-in error message had the same mistake.
- 2026-10-06: File dialog paths are read through `CFileDialogPath::From`, which reads at most
  `nMaxFile` characters. The 21 direct `lpstrFile` reads could run past the buffer when the
  dialog left no terminator (`/analyze` C6054).
- 2026-10-06: Cleared the other step-1 `/analyze` memory findings in our own code:
  - The Slugify table is a static array, no longer a large initializer list on the stack (C6262).
  - The add-in's `PasteAnyAsText` fails visibly when `GlobalLock` returns null.
  - Two findings are verified false positives, suppressed in place with the reason:
    - `CreateQRCodeImage` (C6386): the copy size includes both headers.
    - `BitmapHelper` (C6001): `GlobalReAlloc` is annotated `_Frees_ptr_`, but a failed call
      leaves the block valid.
  - Open: `rijndael.cpp` (dead `EncryptDecrypt`, removed with it), and C6387 null-after-lock
    findings in the app, left for the `GlobalLock` RAII wrapper (Phase C). Findings in the
    vendored sqlite3mc and QRCode sources are not touched: sqlite stays vendored, and QRCode is
    replaced in Phase E.
- 2026-10-06: The RTF editor streams through `CRichEditUtf8Source` and `CRichEditStringSink`.
  Upstream mixed UTF-8 bytes with characters, so non-ASCII text loaded in chunks was corrupted,
  and characters split across chunks were broken.
- 2026-10-06: Deleted dead code: `CTextFile`, `CStdioFileEx`, and the editor's
  `CRulerRichEditCtrl::Save`/`Load`. `Load` called StreamIn with no callback set.
- 2026-10-06: `GetScreenWidth`/`GetScreenHeight` no longer call `GetVersionEx` with an
  uninitialized struct. Every supported Windows is NT.
- 2026-10-06: Windows 10 1607 or later only; the code paths for older Windows are deleted:
  - Clipboard capture uses `AddClipboardFormatListener` directly. The `SetClipboardViewer`
    chain (`WM_DRAWCLIPBOARD`, `WM_CHANGECBCHAIN`) is gone. A failed registration now shows an
    error balloon. Before, it went unnoticed and no copy was saved.
  - The tray icon class uses the SDK `NOTIFYICONDATA`, instead of its own per-shell-version
    structs, its `DllGetVersion` check and the branches for each shell version.
  - `GetDpiForWindow`, `SetLayeredWindowAttributes` and the suspend/resume notification API are
    called directly, not looked up with `GetProcAddress`. A failed power-notification
    registration is shown to the user.
  - `IsVista`, SendKeys' Win9x NumLock code and the unused `CSystemTray` and `U3Stop` are deleted.
- 2026-10-06: Every project builds with `/sdl` and Control Flow Guard, from
  `Directory.Build.targets`.
  - `/sdl` turned one deprecated call into an error (`GetVersion` in SendKeys, removed with the
    Win9x code).
  - It also turned the CRT deprecation warnings in the vendored sqlite3mc amalgamation into
    errors. That one file defines `_CRT_SECURE_NO_WARNINGS`, so SQLite stays unmodified.
- 2026-10-06: Deleted upstream's ARM64 and portable installer scripts. The fork builds neither.
  The ARM64 script still added firewall rules for TCP 23443 and launched URLs, and the portable
  one packaged files that no longer exist (`DittoU.exe`, `sqlite3.dll`, `zlib1.dll`). The
  installer gate now checks every `.iss` file.
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

