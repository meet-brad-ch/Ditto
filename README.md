# Ditto, local-only fork

**What:** A fork of the Ditto clipboard manager (sabrogden/Ditto) that is built locally, with all
network code removed from the build.

**Why:** For safety. The owner has used Ditto for years. A clipboard history holds passwords and
private text, so the copy the owner runs is compiled from source they can read, and it has no
code that can send anything off the machine.

**Status:** working. All network code listed below is removed, the build is clean, and
`tools/verify.ps1` passes. At run time, `tools/runtime-netcheck.ps1` sees no TCP/UDP endpoint owned
by Ditto. The owner runs an installed build of the fork.

What this fork removed (found by reading upstream a80fd35):

- **Friends.** Winsock TCP send and receive of clips, with the server on port 23443. The server
  starts by default on a normal install.
- **HTML clip preview.** It uses the IE WebBrowser control, which fetches remote content inside
  copied HTML.
- **Hand-offs to other programs.** URLs go to the browser through ShellExecute: Help, Web Search,
  Translate, Gmail, the QR URL and links. Email goes to the mail client through MAPI.

- **ChaiScript.** The on-copy and on-paste script engine and its editor (Options → Advanced) are
  removed: an embedded interpreter running user scripts on every clip is attack surface the
  owner does not use.

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

**Verify:** `powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 [-Analyze]`
runs these stages and exits 1 on the first failed one. It ends with the sw-quality §38
Verification block (Build, Unit tests, Lint, Static analysis, Cyclomatic complexity, Line and
Branch coverage), on failure too. Run time: about 2 min, or about 3.5 min with `-Analyze`.

1. It installs the `vcpkg.json` dependencies, then rebuilds Release|x64 with a log
   (`build\logs\build.log`). The rebuild is full because an incremental build reports only the
   warnings of the files it recompiles.
   - **Warnings ratchet:** every project builds at `/W4`. The build's warnings, counted per
     (code, file), may not exceed `tools\baselines\warnings.tsv`.
   - **Code analysis ratchet (`-Analyze`):** the same rebuild runs `/analyze` with
     NativeRecommendedRules. Its findings may not exceed `tools\baselines\analyze.tsv`.
     Without `-Analyze`, static analysis is NOT VERIFIED.
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
   - **Complexity:** lizard measures our own C/C++ code (vendored sqlite, QRCode and TinyXml
     excluded). No function in `lib\` or `tests\` may reach CC 10. The legacy functions at
     CC 10 or more are listed in `tools\baselines\complexity.tsv`, and none may get worse or be
     added.
7. It runs every GoogleTest in `tests\` on its own (`--gtest_filter`), under AddressSanitizer. Each
   test writes its result to `build\test-results\<test>.xml`.
   - **Coverage:** Microsoft code coverage (it ships with Visual Studio) runs the Debug|x64 test
     build. The Release test build uses AddressSanitizer, which the instrumentation does not
     combine with. `lib\DittoCore` must have at least 90 % line coverage, and every uncovered
     line is listed. The tool gives no branch data, so branch coverage is reported as NOT
     VERIFIED.
8. It runs Doxygen (`tools\Doxyfile.contract`): every class, function and member of the contract
   code must be documented, and any Doxygen warning fails. The files are listed by name in that
   Doxyfile.

`-SkipBuild` skips stage 1, so the warnings and analysis ratchets are NOT VERIFIED.

**Baselines** (`tools\baselines\*.tsv`, read by `tools\ratchet.ps1`):
- They hold today's legacy debt: 592 `/W4` warnings, 308 analysis findings and 179 functions at
  CC 10 or more.
- A count above its baseline fails, and so does an entry missing from it.
- `verify.ps1 -UpdateBaselines` rewrites them only when nothing rose, so a baseline can only
  shrink. Commit the smaller file after a cleanup.

**Planted faults the gates caught:**
- a seeded `WSAStartup` line and a copied `curl.exe`;
- binaries built before Control Flow Guard was on (hardening stage);
- the old ARM64 installer script (installer stage);
- a C4244 warning and a C6011 null dereference in `src\ErrorReport.cpp`;
- a 9-branch function in `src\` and in `lib\DittoCore`;
- coverage measured against a raised minimum of 99 %;
- a C4244 committed on a temporary branch, caught by `tools\ci.ps1 -Ref <branch>`. On its first
  real runs `ci.ps1` also found two problems that working-tree runs had hidden: a stale `#import`
  header hiding two warnings, and the installer check skipping the clone.

**Local CI:** `powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1 [-Ref <commit>]`
runs the same job as `.github\workflows\build.yml` on this machine.
- **Clean clone:** it clones the commit into `build\ci\<commit>\work`, so only committed files take
  part. Uncommitted changes are not built; the script says so.
- **Steps:** `verify.ps1 -Analyze`, a Debug|x64 build, then the installer.
- **Output:** `build\ci\<commit>\summary.md` holds each step's result, the §38 block, the
  installer's SHA256 and the per-test table. `build\ci\<commit>\artifacts\` holds the installer,
  the binaries, the test XML, the coverage report and the logs.
- **Cleanup:** the clone is deleted afterwards. The exit code is 0 only if every step passed.
- **Why it exists:** GitHub Actions do not run for this account while it is under a GitHub
  restriction (2026-10-06). The workflow stays in the repo and runs again once Actions work.
  Run `ci.ps1` before every push in the meantime.

**Fuzzing:** `powershell -NoProfile -ExecutionPolicy Bypass -File tools\fuzz.ps1 [-Target <name>]
[-Seconds 60]` runs the libFuzzer targets in `tests\fuzz\` after a Release|x64 build.
- **Build:** `DittoFuzz.exe` is built with `/fsanitize=address,fuzzer` and compiles the DittoCore
  sources itself.
- **Target selection:** one binary holds every target; the `DITTO_FUZZ_TARGET` environment
  variable picks one. Each target's seeds are generated by the target itself into
  `build\fuzz\<target>\corpus`, so no binary inputs are committed.
- **Findings:** a crash, sanitizer report, leak or timeout fails the run, and the triggering input
  is saved as `build\fuzz\<target>\crash-*`. Every finding becomes a unit test before it is fixed.
- **Targets:** `cliptext` (every `ClipText` read) and `hdrop` (`FileDropList::Parse` and
  `GlobalFileDrop::Read`). Each new parser in Phase C adds its own.
- **CI:** `tools\ci.ps1` and the GitHub workflow run every target for 60 s.
- **Red test:** a planted one-byte read past the input was found within a second, with the
  ASan report pointing to the line.

**Runtime network check:** `powershell -NoProfile -ExecutionPolicy Bypass -File
tools\runtime-netcheck.ps1 [-WatchSeconds 20]` runs after a build.
- It runs `Release64` as a portable copy in a new temporary folder and copies four kinds of content
  to the clipboard: text, CF_HTML with a remote `<img>`, an image and a file list.
- It watches the TCP/UDP endpoints the Ditto process owns, and any endpoint fails the check.
- At the end it stops the copy, deletes the folder with its database and log, and restores the
  clipboard text from before the run. With `-WatchSeconds 10` it took 28 s.
- It was tested with a fake `Ditto.exe` that opens a TCP listener, and it failed as intended.
- It changes the clipboard, so a Ditto you are running also records the four test clips.

**Prerequisites:**

- Visual Studio Community 2026 18.9, MSVC 14.51, toolset v145, with the C++ desktop workload.
- The MFC component (`Microsoft.VisualStudio.Component.VC.ATLMFC`). Install it with:
  `setup.exe modify --installPath "C:\Program Files\Microsoft Visual Studio\18\Community" --add Microsoft.VisualStudio.Component.VC.ATLMFC --passive`
  (run from `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer`, elevated).
- Windows SDK 10.0.26100.0.
- vcpkg at `C:\vcpkg`, or set `VCPKG_ROOT`. It must have the baseline commit from `vcpkg.json`
  (verified with vcpkg tool 2025-10-16).
- Doxygen 1.15.0 on `PATH` (or in `C:\Program Files\doxygen\bin`), Python with
  `pip install lizard==1.17.31`, and Inno Setup 7.0.2 for the installer. Code coverage uses
  `Microsoft.CodeCoverage.Console`, which ships with Visual Studio 2026 (Community included).

## Layout

- `src\`: the Ditto app (MFC), mostly upstream code.
- `lib\DittoCore\`: clipboard-format logic in plain C++, written to the contract. It has no MFC, no
  UI and no global state. Built at `/W4 /WX`, with `/sdl`, `/permissive-` and Control Flow Guard
  (`lib\Contract.props`).
- `tests\`: GoogleTest unit tests for DittoCore. The Release build uses AddressSanitizer; the
  output is `build\DittoTests\x64\Release\DittoTests.exe`.
- `tools\verify.ps1`: the quality gate.
- `tools\runtime-netcheck.ps1`: the runtime network check.
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
  - Uninstalling keeps `HKCU\Software\Ditto` (settings and database path). Upstream deleted it.
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
  - Open: C6387 null-after-lock
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
- 2026-10-06: The DittoCore parsers are fuzzed with libFuzzer (Phase C3; see Fuzzing). MSVC
  14.51's fuzzer runtime works with the forced `/guard:cf` and `/sdl`, so the fuzz project needs
  no exemption.
- 2026-10-06: Locked clipboard memory is read through `DittoCore::GlobalBytes` (Phase C2).
  - It locks the block, measures it with `GlobalSize`, and hands out a bounded `std::span`. A
    null handle, a failed lock or an empty block throws, and null is rejected before
    `GlobalLock` (under ASan, `GlobalLock(NULL)` crashes).
  - **Global-memory helpers** (`Misc.cpp`, the add-in's `DittoDefines.h`): they checked sizes only
    with `ASSERT`, which is off in Release. They now check every length against the block.
    `CompareGlobalHH` locked one handle and unlocked the other.
  - **Text reads:** `CClipFormat::GetAsCString/GetAsCStringA` and `CClip::SetDescFromText` read
    up to the first null or the end of the block (`ClipText::ReadAnsiBounded`, new
    `ReadWideBounded`). They no longer drop the last character or carry embedded nulls.
  - **CRC:** `GenerateCRC` no longer runs `strlen`/`wcslen` past the block. CRC values of
    well-formed data are unchanged, so duplicate detection works as before.
  - **CanIncludeInClipboardHistory:** the block must hold a whole DWORD, and it is freed on every
    path.
  - **IStream data:** it is copied only when the whole stream (up to 4 GB) is read. Before, the
    allocation was unchecked, the read count ignored, and the size truncated to 32 bits.
  - **Add-in readers** (`ReadOnlyFlag`, `RemoveLineFeeds`): reads are bounded, the write-back is
    checked, and their export boundaries show a message instead of letting an exception cross
    into Ditto.
  - **CRC table:** freed with `delete[]`, matching its `new[]`.
  - **Baselines:** `/analyze` 308 → 302 (the format-related C6387s), warnings 594 → 593,
    complexity 3461 → 3449.
- 2026-10-06: A multi-clip paste no longer hides errors (Phase C1).
  - `CClipIDs::AggregateData` ended in `catch(...) {}` and a log-and-continue SQLite catch, so a
    malformed clip or a database error during a multi-clip paste vanished silently.
  - Both now reach the paste boundaries: `COleClipSource::OnRenderGlobalData` (it reports and
    returns FALSE) and `CProcessPaste::DoPaste`/`DoDrag` (one guarded run with the message).
  - A database error now shows its code and text instead of "generic exception".
  - `CF_TEXT` multi-paste no longer pulls in file lists unless the paste is text-only. A
    precedence bug had made it always do so.
  - The paste data object is released only after it was handed to the clipboard. Before, it
    leaked when an error came before that.
- 2026-10-06: Every project builds as C++20 (owner decision, Phase C0). `Directory.Build.targets`
  sets the standard once, in place of 29 per-project settings.
  - `/std:c++20` implies `/permissive-`. The contract code (`lib\`, `tests\`) and ICU_Loader keep
    conformance mode on.
  - The legacy MFC code would have 118 conformance errors in 20 files, mostly the vendored colour
    popup, the Advanced Options grid and CppSQLite3. It stays `/permissive` until Phase D.
  - Fixed in the move: `Shared\ArrayEx.h` (two-phase lookup) and `Shared\TextConvert.h`
    (copy-initialization through ATL conversions).
  - The one new C++20 warning (C5054) exposed `CFile::bufferWrite`, a buffer command, used as an
    open flag. It is now `CFile::modeWrite`, which has the same value.
- 2026-10-06: Removed ChaiScript (owner decision): `src\chaiscript\` and its 8 wrapper and
  editor files, plus the script hooks in copy, paste, the paste menu and the shortcut editor.
  - `PASTE_SCRIPT` keeps its number and is marked `Removed`.
  - Saved `CopyScriptsXml`, `PasteScriptsXml` and `QP_ShortCut_94_<guid>_*` settings are left in
    place and ignored.
  - Also removed:
    - the script-only parameters (`refData` on shortcuts and accelerators, the active window
      title on copy);
    - 22 resource IDs;
    - the `/bigobj` option, which only ChaiScript needed; Release and Debug x64 build without it.
  - Fixed a call that passed the active app name as the "check clipboard ignore" flag on the
    copy retry path.
- 2026-10-06: Debug builds use `/Zi` instead of `/ZI`. Edit and Continue cannot be combined with
  the global Control Flow Guard, which had broken every Debug build.
- 2026-10-06: Ditto keeps the clip history when its settings are gone. Before, the uninstaller
  deleted `HKCU\Software\Ditto` (`uninsdeletekey`), including the database path `DBPath3`. On the
  next start Ditto had no path. It created an empty `Ditto_1.db` next to the existing `Ditto.db`,
  so the history looked lost. This happened to the owner on 2026-10-06.
  - With no path set, Ditto now opens `Ditto.db` in the default location
    (`DittoCore::DatabasePath`, 4 tests).
  - The usual checks still apply. A missing file is created, and an invalid one gets the existing
    "Unrecognized Database Format" message.
  - The installer keeps `HKCU\Software\Ditto` on uninstall.
  - Runtime test: a portable copy with `DBPath3` cleared created `Ditto_1.db` before the fix and
    reopens `Ditto.db` after it.
- 2026-10-06: Deleted dead code and files (plan step 4):
  - The `EncryptDecrypt` project. Nothing called it; it was only linked. Its `rijndael.cpp` held
    the last `/analyze` memory finding in our own code.
  - The Friends leftovers in `CMultiLanguage` (two updaters, two language arrays),
    `GetComputerName()`, the empty `CToolTipEx::OnNotify`, and the unused `FILECOPY.AVI` resource.
  - 194 `resource.h` IDs that no code, resource script or language file references.
  - Upstream's release tooling in `DittoSetup`: Chocolatey packages, the GitHub-release
    publisher, the appx Store package (including `my.pfx`), the encrypted `BuildDitto.bld`, the
    portable-zip script, and the committed `ProjectZip.exe` and `rcedit` executables. Only
    `DittoSetup_10.iss` and its translations remain.
  - `ActionEnums::Removed` and `UserConfigurable` are table lookups now (CC 1 and 2, were 23 and
    14).
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

