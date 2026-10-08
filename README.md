# Ditto without network code

A fork of the Ditto clipboard manager with all network code removed. It is built from source and
publishes no releases or downloads.

For what Ditto is, its features and how to use it, see the main project:
**https://github.com/sabrogden/Ditto**. This README covers only what this fork changes and how to
build and check it.

## What this fork changes

### No network code

A clipboard history holds passwords and private text. This build contains no code that can send
anything off the machine:

- **Friends** (clip sharing over TCP port 23443) is removed.
- **HTML clip preview** is removed. It used the IE WebBrowser control, which fetches remote
  content inside copied HTML. HTML clips show as text or RTF.
- **Hand-offs to other programs** are removed: URLs opened in the browser (Help, Web Search,
  Translate, Gmail, the QR URL, links) and email through MAPI.
- **ChaiScript**, the on-copy/on-paste script engine and its editor, is removed.
- `httplib.h` and `sqlite\sqcloud.*` (never compiled upstream) are deleted.

No binary imports a network DLL (`WS2_32`, `WININET` and the others; see Verify). At run time,
`tools\runtime-netcheck.ps1` finds no TCP or UDP endpoint owned by Ditto.

Settings stay compatible with upstream: action numbers are unchanged, so saved keyboard shortcuts
map to the same actions, and network settings in an existing ini file or registry are ignored.

### Code rules, enforced by gates

Every rule below is checked by `tools\verify.ps1` (see Verify):

- **Fail fast.** An operation that cannot continue correctly stops and shows the cause. There are
  no silent fallbacks and no swallowed errors.
- **Smart pointers only.** No raw `new`, `delete`, `malloc` or `free`; `std::unique_ptr`, standard
  containers and RAII wrappers for Win32 resources.
- **No global state.** No free functions, global or static variables, or macros.
- **Complexity.** Every function stays below cyclomatic complexity 10.
- **No warnings.** Every project builds at `/W4 /WX`, `/permissive-`, `/sdl` and Control Flow
  Guard; `/analyze` (NativeRecommendedRules) reports nothing.
- **Formatting.** Every own C/C++ file matches `.clang-format`.
- **Always initialize.** Every variable and member gets a brace initializer (`{}`).
- **Documentation.** Every class, function and member of the contract code has a Doxygen comment.
- **Explicit project files.** Every file is listed by name; no wildcards.
- **No dead code.** Unused code is deleted.

### Architecture

- **`lib\DittoCore\`**: plain C++ with no MFC, no UI and no global state. It holds the clipboard
  format logic (text, RTF, HTML, DIB, file drops, `.dto` export), the settings stores, the search
  SQL, CRC32 (zlib) and MD5 (Windows CNG). It is tested with GoogleTest under AddressSanitizer,
  with a minimum of 90 % line coverage, and fuzzed with libFuzzer.
- **`src\`**: the MFC app. One composition root, `CAppServices`, is the first member of the app
  object and owns every service and the application state: settings (`CGetSetOptions` over an
  `ISettingsStore`: registry, ini file, or in memory for tests), language, database, registered
  clipboard formats, hot keys, windows, clipboard monitor and the others.
- **Access rule:** classes derived from `CCmdTarget` (windows, dialogs, threads, OLE sources) reach
  the services through `theApp.Services()`. Every other class receives its services from its
  caller, through its constructor or a parameter. Two cross-cutting classes, `CLogger` and
  `CErrorReport`, are called from every class and thread and also use `theApp.Services()`.
- **Accepted globals** (`tools\gates\globals-allow.txt`): the MFC `theApp` objects, focus.dll's
  shared-segment hook state, and DLL entry points, hook procedures and exports.

### Third-party code

- **From vcpkg** (`vcpkg.json`, pinned by its `builtin-baseline`): zlib, tinyxml2,
  nayuki-qr-code-generator and gtest.
- **Vendored and untouched** (`tools\thirdparty.txt`): SQLite3 Multiple Ciphers 2.5.1 (SQLite
  3.53.4), SQLite's ICU extension (`ICU_Loader\icu.c`, built against Windows' ICU), the ruler
  editor's colour picker and font/size combo boxes, `WildCardMatch` and `SnapWindow.cpp`. They
  are never edited (only replaced by a newer release), their warnings and code analysis are
  switched off per file, and the gates skip them.
- Third-party code that Ditto changed (for example `CppSQLite3`, `NTray`, the ruler editor) counts
  as Ditto's own code and follows the rules above.

### Installer

Per user, no administrator rights, English only (see Installer).

## Build

Run from the repo root in PowerShell:

```
C:\vcpkg\vcpkg.exe install --triplet x64-windows-static-md --x-install-root=vcpkg_installed\x64-windows-static-md
$msb = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msb CP_Main_10.sln /p:Configuration=Release /p:Platform=x64 /m
Release64\Ditto.exe
```

The build writes `Release64\Ditto.exe`, `Release64\Addins\DittoUtil.dll`, `ICU_Loader.dll` and
`focus.dll`. `tools\verify.ps1` runs the same build and then every gate.

- **vcpkg:** `Directory.Build.props` and `.targets` connect vcpkg to every project with the
  `x64-windows-static-md` triplet (static libraries, dynamic CRT). Auto-link is off, so each
  project lists the libraries it links. Run `vcpkg install` first: under `/m`, each project would
  otherwise start its own install.
- **Supported OS:** Windows 10 1607 (build 14393) and later. `Directory.Build.targets` sets
  `_WIN32_WINNT` and `WINVER` to `0x0A00`, `/sdl` and Control Flow Guard after each project's own
  settings, so no project can turn them off.

### Prerequisites

- Visual Studio Community 2026 (MSVC 14.51, toolset v145) with the C++ desktop workload and the MFC
  component (`Microsoft.VisualStudio.Component.VC.ATLMFC`). To add MFC, run elevated from
  `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer`:
  `setup.exe modify --installPath "C:\Program Files\Microsoft Visual Studio\18\Community" --add Microsoft.VisualStudio.Component.VC.ATLMFC --passive`
- Windows SDK 10.0.26100.0.
- vcpkg at `C:\vcpkg` (or set `VCPKG_ROOT`), with the baseline commit from `vcpkg.json`.
- Doxygen 1.15.0 on `PATH` (or in `C:\Program Files\doxygen\bin`).
- Python with `pip install lizard==1.17.31`.
- Inno Setup 7.0.2, for the installer.
- clang-format and Microsoft code coverage ship with Visual Studio.

## Verify

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\verify.ps1 [-Analyze]` runs these
stages and stops at the first failure. It ends with a verification block (Build, Unit tests,
Lint, Formatting, Static analysis, Cyclomatic complexity, Line and Branch coverage), also on
failure. It takes about 2 minutes, or about 4.5 minutes with `-Analyze`.

1. **Build:** installs the vcpkg dependencies and rebuilds Release|x64 with a log
   (`build\logs\build.log`). Any compiler, linker or librarian warning fails. With `-Analyze`, the
   rebuild runs `/analyze` and any finding in the log fails.
2. **Imports:** no binary in `Release64` may import ws2_32, wsock32, mswsock, wininet, winhttp,
   urlmon, mapi32, dnsapi, iphlpapi, webio or httpapi.
3. **Hardening:** every binary carries high-entropy ASLR, dynamic base, DEP and Control Flow Guard.
4. **Sources:** no source, `.rc` or `.vcxproj` file uses socket, WinINet, WinHTTP, urlmon, MAPI or
   WebBrowser APIs, network DLL names, or `ShellExecute` of an `http(s)://` URL.
5. **Installer script:** no firewall rules, no URL launches, `MinVersion` of Windows 10 or later.
6. **Code gates** over Ditto's own tracked C/C++ files (untouched third-party files are skipped):
   - allocation (`tools\gates\allocation.ps1`): no raw allocation. A handoff to a framework owner
     is allowed when its line says so: `// ownership: <owner>`;
   - globals (`tools\gates\globals.ps1`): no free functions, global or static variables or macros,
     except the accepted list;
   - complexity (`tools\gates\complexity.ps1`, lizard): every function below CC 10;
   - formatting (`tools\gates\format.ps1`): every file matches `.clang-format` (fix with
     `tools\gates\format.ps1 -Repo . -Fix`). The resource editor's `resource.h` files are skipped.

   The allocation and globals gates compare against `tools\baselines\*.tsv`, which are empty and
   may only shrink.
7. **Tests:** every GoogleTest in `tests\` runs on its own, DittoTests under AddressSanitizer, and
   writes `build\test-results\<test>.xml`. Coverage (Microsoft code coverage, Debug|x64 builds):
   `lib\DittoCore` needs at least 90 % line coverage, and the app layer's line coverage (the
   `src\` files that AppTests compiles) is reported. Branch coverage is not measured.
8. **Documentation:** Doxygen (`tools\Doxyfile.contract`) checks that the contract code is fully
   documented; any warning fails.

`-SkipBuild` skips stage 1 (the warning checks are then not verified).

### Local CI

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1 [-Ref <commit>]` is the
project's CI. Run it before every push.

- It clones the commit into `build\ci\<commit>\work`, so only committed files take part.
- Steps: `verify.ps1 -Analyze`, `fuzz.ps1`, the Debug|x64, Debug|Win32 and Release|Win32 builds,
  then the installer (any Inno Setup warning fails).
- Output: `build\ci\<commit>\summary.md` (each step, the verification block, the installer's
  SHA256, the per-test table) and `build\ci\<commit>\artifacts\` (installer, binaries, test
  results, coverage, logs). The clone is deleted afterwards. The exit code is 0 only when every
  step passed. It takes about 17 minutes.

### Fuzzing

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\fuzz.ps1 [-Target <name>] [-Seconds 60]`
runs the libFuzzer targets in `tests\fuzz\` (`DittoFuzz.exe`, built with
`/fsanitize=address,fuzzer`).

- Targets: `cfhtml`, `cliptext`, `dib`, `dto`, `filedata`, `hdrop` and `rtf`.
- Each target generates its own seeds into `build\fuzz\<target>\corpus`.
- A crash, sanitizer report, leak or timeout fails the run and saves the input as
  `build\fuzz\<target>\crash-*`. Every finding becomes a unit test before it is fixed.

### Runtime network check

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\runtime-netcheck.ps1 [-WatchSeconds 20]`
runs `Release64` as a portable copy in a temporary folder, copies text, HTML with a remote image,
an image and a file list to the clipboard, and fails on any TCP or UDP endpoint owned by the
Ditto process. Afterwards it deletes the folder and restores the clipboard text. It changes the
clipboard, so a running Ditto also records the four test clips.

## Layout

- `src\`: the Ditto app (MFC).
- `lib\DittoCore\`: the contract library (see Architecture), with its settings in
  `lib\Contract.props`.
- `tests\`: DittoTests (GoogleTest for DittoCore), `tests\AppTests\` (the app layer against an
  in-memory SQLite database) and `tests\fuzz\`.
- `tools\`: `verify.ps1`, `ci.ps1`, `fuzz.ps1`, `runtime-netcheck.ps1`, and the gates in
  `tools\gates\`.
- `DittoSetup\`: the Inno Setup installer script.
- `Debug\Language\`, `Debug\Themes\`: the language and theme files the installer packages.

## Installer

Build Release|x64 first, then run:

```
& "C:\Program Files\Inno Setup 7\ISCC.exe" DittoSetup\DittoSetup_10.iss
```

- **Output:** `DittoSetup\Output\DittoLocalSetup_<exe version>.exe`, unsigned. The version comes
  from `Ditto.exe` (`CP_Main.rc`).
- **Per user, no administrator rights:** Ditto installs into `%LOCALAPPDATA%\Programs\Ditto`. The
  start menu entry, the `.dto` file association and all registry writes go to the installing
  user's HKCU.
- **English only:** the setup pages are English. Ditto's language files are installed and chosen
  in Options.
- **Contents:** Ditto.exe, ICU_Loader.dll, Addins\DittoUtil.dll, the VC++/MFC runtime DLLs next to
  Ditto.exe (no redistributable install), and the language and theme files.
- **No network:** no firewall rule and no browser links after setup.
- **Upgrade:** a per-user install is upgraded in place and keeps its settings; a running Ditto is
  closed first. A per-machine install (in Program Files) must be uninstalled first, as an
  administrator. Uninstalling keeps the settings in `HKCU\Software\Ditto`.
- **Crash dumps:** Windows writes its default dumps to `%LOCALAPPDATA%\CrashDumps`. Full dumps
  need the key `HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\Ditto.exe`,
  which needs administrator rights.

## Upstream

- `origin`: https://github.com/meet-brad-ch/Ditto (this fork).
- `upstream`: https://github.com/sabrogden/Ditto (the main project).

This is a hard fork with no merges. To take an upstream fix, fetch `upstream`, read the commit and
port it by hand.
