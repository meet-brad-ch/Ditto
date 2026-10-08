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
   - **No warnings:** every project builds at `/W4 /WX` (`Directory.Build.targets`), with linker
     and librarian warnings as errors too, so any warning fails the build. The build log is
     also checked for any compiler or linker warning. Untouched third-party files are silenced
     per file (see Decisions).
   - **No code analysis findings (`-Analyze`):** the same rebuild runs `/analyze` with
     NativeRecommendedRules. `/WX` does not make every finding an error (C26495 findings left
     the build passing), so the build log check is what fails on a finding. Without
     `-Analyze`, static analysis is NOT VERIFIED.
2. It runs `dumpbin /imports` on every `.exe`/`.dll` in `Release64`. No binary may import
   ws2_32, wsock32, mswsock, wininet, winhttp, urlmon, mapi32, dnsapi, iphlpapi, webio or
   httpapi.
3. It runs `dumpbin /headers` on the same binaries. Each must carry high-entropy ASLR, dynamic
   base, DEP (NX) and Control Flow Guard.
4. It greps every source, `.rc` and `.vcxproj` file for socket, WinINet, WinHTTP, urlmon, MAPI and
   WebBrowser APIs, network DLL names and `ShellExecute` of an `http(s)://` URL.
5. It checks every Inno Setup script (`*.iss`): no firewall rules (netsh), no URL launches, and a
   `MinVersion` of Windows 10 or later.
6. It checks Ditto's own C/C++ code (tracked files; the untouched third-party files in
   `tools\thirdparty.txt` are skipped by every gate). Contract code (`lib\`, `tests\`) must be
   clean; legacy code is held by baselines in `tools\baselines\` that may only shrink
   (`-UpdateBaselines` rewrites them after a passing run).
   - **Allocation** (`tools\gates\allocation.ps1`): no raw `new`, `delete`, `malloc`, `free`. A
     handoff to a framework owner is allowed when its line says so: `// ownership: <owner>`.
   - **Globals** (`tools\gates\globals.ps1`): no free functions, global or static variables, or
     macros (include guards and `resource.h` excepted). The accepted exceptions, each with its
     reason, are in `tools\gates\globals-allow.txt`: the MFC `theApp` objects, focus.dll's
     shared-segment hook state, and DLL entry points, hook procedures and exports.
   - **Complexity** (`tools\gates\complexity.ps1`, lizard): no function of Ditto's own code
     (`lib\`, `tests\` and the app alike) may reach CC 10. Untouched third-party files
     (`tools\thirdparty.txt`) are not measured.
   - **Formatting** (`tools\gates\format.ps1`): every own C/C++ file matches `.clang-format`
     (tabs, Allman braces, existing line breaks kept, includes never sorted), checked with the
     clang-format that Visual Studio ships (22.1.3 here; another version may format differently).
     Untouched third-party files and the resource editor's `resource.h` are not formatted. Fix
     with `powershell -NoProfile -ExecutionPolicy Bypass -File tools\gates\format.ps1 -Repo . -Fix`.
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

`-SkipBuild` skips stage 1, so the warnings and analysis checks are NOT VERIFIED.

**Allocation and globals baselines** (`tools\baselines\allocation.tsv`, `globals.tsv`, read by
`tools\ratchet.ps1`):
- They hold the per-file counts of the legacy code (allocation: now empty).
- A count above its baseline fails, and so does an entry missing from it.
- `verify.ps1 -UpdateBaselines` rewrites them only when nothing rose, so they can only shrink.
  Commit the smaller files after a cleanup.

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
- **Steps:** `verify.ps1 -Analyze`, `fuzz.ps1`, Debug|x64, Debug|Win32 and Release|Win32
  builds (all `/W4 /WX`), then the installer. Together with `verify.ps1`'s Release|x64 build
  that is every configuration of the solution.
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
- **Per user, no administrator rights** (owner decision 2026-10-07): Ditto installs into
  `%LOCALAPPDATA%\Programs\Ditto`, its start menu entry and the `.dto` association
  (`HKCU\Software\Classes`) are the installing user's, and every registry write goes to that
  user's HKCU.
- **English only** (owner decision 2026-10-07): the setup's own pages are English. Ditto's
  language files are still installed and chosen in Options; the setup no longer writes
  `LanguageFile`, so a reinstall keeps the chosen language.
- **An earlier per-machine install** (Program Files, from upstream's or an earlier fork
  installer) stops the setup with a message: uninstall it first, as an administrator. The
  settings in `HKCU\Software\Ditto` survive that uninstall.
- **Crash dumps:** Windows Error Reporting's `LocalDumps` settings live under HKLM only and need
  administrator rights, so the setup no longer registers them. Without them WER writes its
  default dumps to `%LOCALAPPDATA%\CrashDumps`; to keep full dumps, add the key
  `HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\Ditto.exe` as an
  administrator.
- **What it packages:** Ditto.exe, ICU_Loader.dll, Addins\DittoUtil.dll, the VC++/MFC runtime
  DLLs from System32 (14.51) next to Ditto.exe (app-local, no redistributable install), and
  `Debug\Language` and `Debug\Themes`.
- **What changed from upstream's script:**
  - The netsh firewall task (TCP 23443) is removed.
  - The post-install "View Help" and "View Change History" browser links are removed.
  - The publisher names the fork.
  - The runtime DLL folder works with both 32-bit and 64-bit ISCC.
  - `{pf}` is now `{autopf}` with `PrivilegesRequired=lowest` (per user, see above).
  - The VC++ runtime check, the empty pre-install steps, the cleanup of upstream's pre-2017 DLLs
    and the HKLM crash-dump registration are removed, as are the eight unofficial translations.
  - `MinVersion` is Windows 10 1607 (was Windows 7). The Windows 7/8 `cmd.exe` paste strings are
    no longer written; the leftover values are deleted on every install.
  - Uninstalling keeps `HKCU\Software\Ditto` (settings and database path). Upstream deleted it.
- **Existing install:** the AppName is still "Ditto", so the installer upgrades an earlier
  per-user install in place and keeps its settings. It closes a running Ditto while it installs.
- **Warnings:** none. `tools\ci.ps1` and the workflow fail on any Inno Setup warning, as on a
  compiler warning.

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
- 2026-10-06: Fixed the remaining Phase C findings (C12):
  - **RTF preview drawing** (`CFormattedTextDraw`): the RTF was converted into an unchecked
    `malloc` buffer of the character count, left unterminated for `lstrlenA`, and the stream
    callback copied from the start every time, so RTF longer than one chunk repeated its
    beginning. It now streams a `std::string` sized by the conversion, from the read position.
  - **Format names:** a failed `GetClipboardFormatName` returned an uninitialized buffer as the
    name; the buffer is zero-initialized now, so the name is empty.
  - **Image join:** the window DC taken for joining images was never released, and the image
    blocks were freed only after a successful join.
  - **Paste any as text** (add-in): every null was turned into a space, the terminator too, so
    the text had none. Trailing nulls are now dropped and one terminator added; the old block
    is freed with the other formats.
  - **Dead code:** `CClip::WriteTextToHtmlFile` (always returned false, never called).
- 2026-10-06: The database connection is a locked `CDittoDb` (Phase C11, first part).
  - **Shared connection:** the clip window, the copy thread and the paste and import paths share
    one SQLite connection. An insert and its `lastRowId()` were two calls, so an insert by
    another thread in between gave a clip or format the wrong id. `InsertReturningId` runs both
    under one recursive lock; `AddToDB` holds the lock from reading the newest order to writing
    the clip.
  - **Transactions:** `CDittoDbTransaction` (RAII; rolled back unless committed). Saving a clip
    writes its Main row and Data rows in one transaction (before, a failed data insert left an
    empty clip in the list), and saving from the editor deletes and rewrites the data in one
    (before, a failure lost the clip's contents).
  - **Bound values:** the clip and group writes bind their values. Upstream doubled the quotes
    of the description and quick-paste text in memory, printed the orders with `%f` (6
    decimals, so repeated moves between two clips stopped working) and the group date as a
    32-bit `int`.
  - **Moving clips:** the eight copies of the move-up/move-down query are one `Move`, and the
    order rules are `DittoCore::ClipOrder` (7 unit tests).
  - **Fixed:** a format saved without data reused the previous format's memory, so two formats
    freed one block (it is now left out and logged); `LoadFormat` returned `false` as a handle;
    clearing another clip's top-sticky setting always reported "not changed".
- 2026-10-07: Compiler warnings and `/analyze` findings are fixed, not listed (owner decision,
  Phase W). Third-party code is split by one rule: a file whose code was changed for Ditto after
  it was imported (by upstream Ditto or by this fork) is Ditto's code and gets fixed; a file
  never changed in code (only imported, moved, re-formatted, or replaced by a newer release of
  its library) is untouched, and its warnings and code analysis are switched off. Measured with
  `git log --follow -p -w` per file:
  - **Untouched, silenced per file in the project files:** the sqlite3mc amalgamation,
    libqrencode (`src\QRCode\*`), TinyXML's `tinyxmlparser.cpp`,
    `tinystr.cpp` and `tinyxmlerror.cpp`, and the ruler editor's `ColourPicker`, `ColourPopup`,
    `FontComboBox.cpp`, `SizeComboBox.cpp` and `StdGrfx.cpp`.
  - **Changed for Ditto, fixed like our code:** `tinyxml.cpp/.h` (Unicode paths), `Path`,
    `memdc.h`, `EditWithButton`, `DrawHTML` (the C++ class `HtmlTextDrawer` since Phase L2), `GdipButton`, `SymbolEdit`, `SendKeys`,
    `CppSQLite3`, `NTray`, `AlphaBlend`, `FormattedTextDraw`, `RulerRichEditCtrl`, `RRECToolbar`,
    `RulerRichEdit` and `ICU_Loader\icu.cpp` (replaced in Phase T by SQLite's untouched `icu.c`).
  - **Headers outside the repo** (Windows SDK, MFC, the STL, vcpkg's gtest and zlib) are
    included with angle brackets and treated as external: no warnings, no analysis.
  - **Silencing analysis per file:** besides `EnablePREfast=false`, the ruleset options are
    cleared for those files, because cl lets a later `/analyze:ruleset` override `/analyze-`
    (warning D9025; checked with a probe compile).
  - **Deleted:** `src\QRCode\QRGenerator.cpp`, libqrencode's sample program (a `_tmain` that
    Ditto never calls).
  - **The fixes** (about 850 sites, details in the commits) also fixed real bugs: an assignment
    in `HideQPasteWindow`'s condition ignored callers that asked to keep the search view; the
    HTML-drawing tag table held wide strings in `char*` fields, so tag lookups read past them;
    `CppSQLite3Query` left its database pointer unset; several error messages lost their
    reason (`%s` missing, narrow strings in a wide `%s`); the list's text colour was never
    restored after drawing.
  - **Event threads:** the UAC events get a DACL for Authenticated Users instead of a NULL DACL
    (anonymous access); stopping a thread no longer kills it with `TerminateThread` after 5 s
    (it could leave the database or heap lock held), it waits and logs which thread is late.
  - **Gate:** every project builds with `/W4 /WX`, and `verify.ps1` fails on any warning or
    code analysis finding in the build log (`/WX` lets C26495 findings pass); the warnings and
    analysis ratchets are gone.
  - **Every configuration:** Debug|x64 and the Win32 configurations showed warnings Release|x64
    did not (Debug-only code, 32-bit `size_t`); they are fixed, and CI builds all of them. The
    ARM64 configurations are removed (owner decision): this machine has no ARM64 compiler or
    ARM64 MFC to build or check them, and no installer ships them.
- 2026-10-07: After a `sw-quality review all`, the owner chose to do all of the remaining work
  (Phases G1, G2, G3, F, E, D).
  - **G1, defects found in Phase W, each with a regression test where testable:** the copy
    properties dialog released the move-to-group hot key by the paste key
    (`CClipRepository::ReleaseShortCuts`); database backup and restore leaked, ignored read and
    write errors and swallowed exceptions (now `DittoCore::GzipStream`, tested);
    `CppSQLite3DB::open` leaked the handle on failure and loaded ICU unchecked into every
    connection (now `loadExtension`, main connection only, C API only); save dialogs had cut
    filters and `OFN_FILEMUSTEXIST`; tool tip buffers were freed with the wrong `delete`; a
    damaged font setting stopped Ditto at every start; and smaller ones (see the commits).
  - **G2, installer:** per user without administrator rights, English only, zero Inno Setup
    warnings, enforced by CI (see Installer).
  - **G3, error handling:** the 73 `CATCH_SQLITE_EXCEPTION` sites that logged a database error
    and went on now report it to the user (`CErrorReport`) and stop the operation; the macros
    are gone. `InitInstance` is the application-start boundary.
  - **F, settings injection:** `CClip` gets its save settings as a
    `DittoCore::ClipSaveSettings` record through `ClipSavePolicy` (tested) instead of reading
    `CGetSetOptions`; the default constructor reads them once from the options.
  - **E, QR code:** `DittoCore::QrBitmap` (tested) renders with vcpkg's
    `nayuki-qr-code-generator`; the vendored libqrencode is removed.
  - **D, conformance:** every project builds with `/permissive-`. The conformance errors in
    Ditto's code (about 40 sites) were fixed: string literals passed to non-`const` parameters,
    qualified member declarations, a `CPath` copy-initialised into a `CString` (two user
    conversions), an ambiguous ternary and `bind`, and a dead `CGdipButton::Test`. The untouched `ColourPopup.cpp` keeps `/permissive`
    and compiles without the shared precompiled header, which a different conformance mode
    cannot use.
- 2026-10-07: The collected defects are fixed, failures are visible, dead code is gone (Phase R).
  - **Owner decision:** every clear defect is fixed, with a regression test where the code can be
    tested; behaviours that may be intended are left for the owner (listed in the session log).
  - **Failures:** callers act on the failure results of Phase G3; moves, deletes and the clip
    types run in transactions; a database that is only locked or in use is never renamed to
    `_BAD` or deleted (`CppSQLite3Exception::isUnavailable`, `DittoCore::DatabasePath`); the
    schema upgrade uses `IF [NOT] EXISTS` instead of swallowing every error.
  - **Prepared statements are locked now:** `CDittoDb::connectionMutex()` gives every
    `CppSQLite3Statement` the connection lock for prepare, step and reset, so they wait for
    another thread's open transaction like `execDML`/`execQuery` (this replaces the 2026-10-06
    note below; test `DittoDb.PreparedStatementWaitsForAnotherThreadsTransaction`). Binding,
    later rows of a query and finalize are not locked.
  - **Threads:** log writes are serialised; the app-running flag, the copy thread's config flag
    and the last-added clip are thread-safe; the paste-time flag travels with the background
    update.
  - **Search:** the SQL lives in `DittoCore::SearchCondition` (tested).
  - **Removed as dead:** `CSelectDB`, `COptionsUtilities` and the no-op compact/repair functions
    (their only real implementation was DAO, before SQLite), the obsolete `DittoUtil.vcproj/.sln`,
    and many unused members, handlers and commented-out blocks.
- 2026-10-07: No globals in Ditto's own code: full OOD with one composition root (Phase L3).
  - **Macros (L3a):** every `#define` of the own code (448) became a class-scoped `enum`,
    `static constexpr` member or static member function; configuration macros moved to the
    projects' `PreprocessorDefinitions`; `UnicodeMacros.h` and the dead `DEBUG_NEW` blocks are gone.
  - **Free functions and globals (L3b):** grouped into classes (`CStringUtil`, `CLogger`,
    `CFileSystem`, `CMonitorGeometry`, `CDatabaseManager`, `nsPath::CPathUtil`, ...); Windows and
    SQLite callbacks are static members; per-window state moved into its object.
  - **Settings (L3c):** `CGetSetOptions` is an instance over `DittoCore::ISettingsStore`
    (`RegistrySettingsStore`, `IniSettingsStore`, `InMemorySettingsStore`, tested in DittoTests).
  - **Composition root (L3d):** `CAppServices`, the first member of `CCP_MainApp`, owns every
    service and the application state (settings, language, database, registered clipboard formats,
    hot-key registry, `CAppState`, windows, clipboard monitor, groups, ...). `theApp` keeps only
    the MFC overrides, the frame lifecycle hooks and `Services()`.
  - **Access rule (owner decision "Hybrid"):** classes derived from `CCmdTarget` (windows,
    dialogs, threads, OLE sources) reach the services through `theApp.Services()`; every other
    class gets them from its caller (constructor or parameter; `CClip` through `CClipContext`).
    Two documented exceptions, cross-cutting and used from every class and thread: `CLogger` and
    `CErrorReport` read `theApp.Services()`.
  - **The accepted globals** (`tools\gates\globals-allow.txt`): `theApp`, focus.dll's
    shared-segment hook state, DLL entry points, hook procedures and exports. The globals gate
    finds nothing else.
- 2026-10-07: The legacy app meets the smart-pointer and complexity rules (Phases L1, L2).
  - **Smart pointers only (L1):** no raw `new`/`delete`/`malloc`/`free` is left in own code
    (allocation gate 223 → 0). Self-deleting MFC windows are handed to the window before
    `Create`, because `CWnd::CreateEx` calls `PostNcDestroy` itself when creation fails; the line
    says so with `// ownership:`. DYNCREATE frames use their `CreateObject` factory.
    `IClipFormat::CreateGdiplusBitmap` keeps its raw-pointer add-in ABI; Ditto calls
    `LoadGdiplusBitmap` (`std::unique_ptr`).
  - **Complexity below 10 everywhere (L2):** the 152 legacy functions at CC 10 or more (total
    3056; worst `CAdvGeneral::OnBnClickedOk` 185) are split into private member functions and
    `static constexpr`/`static const` tables (actions, settings, hot keys, format names), with
    the behaviour unchanged. The complexity baseline is deleted: the limit applies to all own
    code. Free functions that needed helpers forward to a class (`PathRootParser`,
    `DatabaseLocator`, `DatabaseSchemaUpgrader`, `CMarkerInserter`, `CMenuPopupUpdater`); the
    forwarders go in Phase L3. `DrawHTML.C` became the C++ class `HtmlTextDrawer`.
- 2026-10-07: Third-party code is replaced by maintained releases where one exists (Phase T,
  owner decision); untouched third-party code is otherwise never changed.
  - **sqlite3mc:** SQLite3 Multiple Ciphers 2.5.1 (SQLite 3.53.4), was 2.3.5 (3.53.2).
    ICU_Loader's `sqlite3.h`/`sqlite3ext.h` come from the same release.
  - **ICU extension:** SQLite 3.53.4's `ext/icu/icu.c`, byte for byte and untouched; shims in
    `ICU_Loader\unicode\` point its ICU includes to Windows' `<icu.h>`. Ditto's former changes
    moved out: the regex search builds case-insensitivity and "find anywhere" into its pattern
    (`(?i)(?s:.*)(?:…)(?s:.*)`), so the DLL has no global flag and no extra export.
  - **XML:** vcpkg's tinyxml2 replaces TinyXML 2.6.2 (language files, themes, search history);
    `CXmlFile` opens Unicode paths. The search history is UTF-8 both ways now (non-ASCII searches
    were garbled).
  - **CRC and MD5:** `DittoCore::Crc32` (zlib) and `DittoCore::Md5` (Windows CNG) replace the
    copied `Crc32Dynamic` and `Md5`; both give the values Ditto stored before (tests pin them).
  - **Removed:** `PerfTimer` (`std::chrono` in its one use) and the unused `DIBAPI.H`.
  - **Still untouched third-party:** the ruler editor's `ColourPicker`, `ColourPopup`,
    `FontComboBox`, `SizeComboBox`, `StdGrfx`; `WildCardMatch`; `SnapWindow.cpp`; the sqlite3mc
    amalgamation; SQLite's `icu.c`.
- 2026-10-06: The clip SQL is in `CClipRepository`, tested by AppTests (Phase C11, second part).
  - **Repository:** `CClipRepository` takes the database as a parameter and works on plain
    records (`ClipRecord`, `FormatRecord`), so it builds and is tested without `CClip` or the
    app. `CClip` converts its members and formats to records and back. Every value is bound;
    the order columns are an enum, never text from outside.
  - **AppTests:** a new MFC GoogleTest program (`tests\AppTests`) tests the repository and the
    database lock against an in-memory SQLite database (17 tests). `verify.ps1` runs the tests
    of both programs, each test on its own.
  - **Nested transactions:** part 1 made saving a clip a transaction, but copying clips to a
    group already ran inside one, and SQLite cannot nest `BEGIN`, so the copy would have failed.
    An inner `CDittoDbTransaction` is a savepoint now. The three remaining manual
    `begin`/`commit` pairs (copy to group, `SaveFormats`, the editor's save) became
    transactions; upstream left them open when a statement threw. The editor's save no longer
    keeps a transaction open while its properties dialog is shown.
  - **Every statement waits for an open transaction:** `execDML`, `execQuery` and `execScalar`
    are virtual and locked in `CDittoDb`, so a statement from another thread no longer runs
    inside (and is rolled back with) a transaction it does not belong to. Prepared statements
    run without the lock, except inserts through `InsertReturningId`.
  - **Settings stay global:** the duplicate options and the sound are still read from
    `CGetSetOptions` in `CClip`; injecting them waits for Phase F.
- 2026-10-06: The special-paste transforms live in DittoCore (Phase C10): `CaseTransforms` (with
  an injected `ICaseMapper`, ICU in the app), `TextTransforms`, `RtfTransforms`, `Typoglycemia`
  (with an injected `IRandomRange`) and `Slugifier`. `OleClipSource` keeps one helper that reads
  the clip's text, applies a transform and writes it back, instead of a copy of that code for
  every transform and format.
  - **One text, both formats:** the transform runs once on the Unicode text, and CF_TEXT is
    written from the result, so the two formats always match. Before, each format was changed
    on its own: upper and lower case wrote UTF-16 bytes into CF_TEXT, slugify and typoglycemia
    left CF_TEXT unchanged, and the date line had one line break in CF_UNICODETEXT but two in
    CF_TEXT.
  - **Fixed:** camel case wrote stale characters after the shortened text; sentence case missed
    the first word after a period at the end of a line; typoglycemia added a trailing space;
    slugify dropped its trim (separators at both ends) and threw `std::regex_error` for a
    separator such as `]`; the RTF date line was not escaped and left `\r\n\r\n` after the
    closing brace.
  - **ICU:** upper- and lower-casing a text sized ICU's output at 1.2 times the input and
    ignored ICU's error, so a short text that grows (German sharp s becomes "SS") was cut off.
    The length is now asked for first and errors are raised. Without icu.dll the character
    functions use the wide C runtime functions instead of `isupper` on a `wchar_t`.
  - **Removed:** `src\Slugify.h` (free functions in a header) and the drive-letter helpers.
  - **Tests:** 22 unit tests.
- 2026-10-06: Exported `.dto` files are compressed and read by `DittoCore::DtoCodec` (Phase C9).
  - **Untrusted size:** a `.dto` file stores each format's original size, and upstream
    allocated that many bytes (`new Bytef[lOriginalSize]`) before looking at the data, so a
    small file could make Ditto allocate gigabytes. The size is now refused when it is
    negative, above 1 GiB, or more than deflate can produce from the stored bytes, and the
    result must have exactly that size.
  - **Errors are shown:** a format that does not uncompress stops the import with a message.
    Before, it was logged and the clip was imported without that format. An export that
    cannot compress a format stops with a message instead of writing an empty format.
  - **Import count:** the "Successfully imported N clip(s)" message assigned 1 to the count
    in its condition (`m_importCount = 1`), so it always said "clip".
  - **Tests:** 9 unit tests; fuzz target `dto`.
- 2026-10-06: Multi-clip text and file lists are joined by `DittoCore::TextJoin`, and CF_HDROP
  blocks are built by `DittoCore::FileDropList::Build` (Phase C8).
  - **`CFileRecieve` is gone:** it built CF_HDROP in a raw `new TCHAR[]` buffer (a byte count
    used as a character count, so twice the size needed). Its send/receive logging and the
    `LogSendReceiveErrors` option, left over from the removed network code, are gone with it.
  - **Separators:** a separator now goes only between items. Before, it was added after every
    clip except the last by position, so a skipped last clip (an empty file list) left a
    separator at the end of the pasted text.
  - **Import:** a clip's CF_UNICODETEXT written back to the clipboard on import got 1
    terminating byte instead of a whole 2-byte character, so readers could run past the text.
  - **Tests:** 10 unit tests; the `hdrop` fuzz target also checks a Build/Parse round trip.
- 2026-10-06: Multi-clip RTF paste is joined by `DittoCore::RtfJoin`, and the RTF
  normalization for duplicate detection lives in `DittoCore::RtfNormalizer` (Phase C7).
  - **Three or more clips:** upstream removed `{\rtf1` from the last document only, so every
    middle document kept its opening brace without its closing one, and the pasted RTF was
    unbalanced. Now each further document is inserted without its own outer group.
  - **Separator:** it is escaped as RTF (backslash, braces, `\uN?` for characters outside
    ASCII). Before, it went in unescaped in the ANSI code page, and a line break became `\par`
    without a delimiter, so a separator line starting with a letter merged into the control word.
  - **Malformed RTF:** a clip that does not start with `{\rtf1` or has no closing brace stops
    the paste with a message. Before, only the last clip was checked.
  - **Normalizer:** upstream's rules are kept exactly (`{\*\datastore}`, `\rsid`, `\insrsid`,
    `\mdispDef1`), so CRCs of clips already in a database still match new copies.
  - **Tests:** 15 unit tests; fuzz target `rtf`.
- 2026-10-06: Contract code (`lib\`, `tests\`) is compiled with `/utf-8`. Without it, the
  BOM-less sources were read in the ANSI code page, so the non-ASCII test strings (the emoji
  and check marks in the CF_HTML tests) were mangled the same way on both sides of each
  comparison, and the tests did not test those characters.
- 2026-10-06: The clips made by *Save copied file (cf_hdrop) contents into Ditto* ("Ditto File
  Data") are read and written by
  `DittoCore::FileDataRecord` (Phase C6).
  - **Every file kept:** upstream added one format per copied file, and each replaced the one
    before, so a clip of several files kept only the last file's contents while its
    description listed them all. A new version-2 record holds all files with their lengths.
    Version-1 records (one file) in existing databases and `.dto` files are still read.
  - **Formats saved twice or lost:** upstream removed the clip's old formats with
    `RemoveAt(i)` on a shrinking array, so it skipped every other format, saved some again, and
    could remove the file data instead.
  - **Pasting checks the record:** lengths and terminators are checked against the block, and
    a failed MD5 check or a file that cannot be written stops the paste with a message. Before,
    the block was read without bounds and both failures were only logged, so files went
    missing silently.
  - **Same names:** two files with the same name are pasted as `name.ext` and
    `name (2).ext`; before, the second overwrote the first.
  - **Reading files:** the file is read into a vector with its size checked against the
    maximum before any cast; the read count is checked; the `new[]` buffer that leaked per file
    is gone.
  - **Tests:** 14 unit tests (`FileDataRecord`, `ByteCursor`); fuzz target `filedata` with a
    Parse/Build/Parse round trip.
- 2026-10-06: CF_DIB images are checked by `DittoCore::DibHeader` before they are drawn or
  saved (Phase C5). It checks the header size (40, 52, 56, 108 or 124 bytes), planes, bit count,
  compression, color count, and that the color table and pixels fit in the block.
  - **Pixel offset:** the old code assumed a 40-byte header and ignored the three bit masks of a
    `BI_BITFIELDS` image. The image preview, *Export to bitmap file* and the image-to-HTML add-in
    wrote a `.bmp` header pointing at the wrong byte, and the list thumbnail drew the pixels from
    the wrong place with palette indexes (`DIB_PAL_COLORS`) instead of RGB colors.
  - **Malformed images:** a DIB whose sizes do not fit its block now shows a message (thumbnail,
    preview, tooltip, export, save, edit, drag as file, add-in). Before, it was read past the
    end of the block.
  - **Crashes fixed:** combining the images of several clips dereferenced a null bitmap when one
    image could not be loaded; saving an image dereferenced a null image when the data was
    missing.
  - **Tests:** 22 unit tests; fuzz target `dib`, which reads every pixel byte the layout names.
- 2026-10-06: CF_HTML is read and written by `DittoCore::CfHtml` (Phase C4).
  - **Offsets:** the old parser decoded the UTF-8 block with the ANSI code page and used the
    byte offsets as UTF-16 indexes. Non-ASCII text before or in the fragment cut it in the wrong
    place, and the result was converted back through the code page.
  - **Malformed headers:** a CF_HTML block without valid `StartFragment`/`EndFragment` byte
    offsets inside the text now stops the paste with a message. Before, the clip was silently
    left out.
  - **Built blocks:** 10-digit byte offsets, `Version:0.9` when none is known, and `SourceURL`
    only when known.
  - **Separator:** the multi-paste separator is inserted as escaped HTML (UTF-8, line breaks as
    `<br>`). Before, it went in as ANSI text without escaping.
  - **Add-in:** the image-to-HTML add-in writes a real CF_HTML block with a terminator. Before,
    it wrote bare `<IMG>` markup.
  - **Tests:** 14 unit tests; fuzz target `cfhtml` with a Parse/Build/Parse round-trip check.
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

