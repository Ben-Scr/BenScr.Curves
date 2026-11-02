# Verification and release

Created by Ben Schneider — https://github.com/Ben-Scr

## Tools and matrix

- C++20, CMake minimum 3.28, PowerShell 7, packaging SDK 10.0.303.
- Windows CI: windows-2022, Visual Studio 2022/MSVC v143, Release contract tests,
  Debug and Release x64 native NuGet consumers.
- Linux CI: ubuntu-24.04, GCC 14 and Clang 18, CMake 3.31.6, Ninja, ASan and UBSan.
- Local validation: Visual Studio 2026/MSVC 19.51, CMake 4.3.1-msvc1; native NuGet consumer v143.
- Formatting: `.clang-format`, checked locally with clang-format 22.1.3; UTF-8, LF, four spaces.
- No ARM, 32-bit, macOS, fast-math or altered floating-point environment execution is claimed.
  ThreadSanitizer is not configured; concurrent const reads have an executable contract test.

## Contract coverage

18 contract groups include known built-in values, endpoint/clamp behavior, invalid values,
bounded output close to one, identity and degenerate Bezier controls, tiny/subnormal inputs,
stationary-midpoint neighbors, time-vs-parameter distinction, independent polynomial values,
overshoot and extreme finite controls, track validation, empty/single-key behavior, exact key
times, step transitions, extreme time/value ranges, representable and overflowing extrapolation,
deep copies, move/self-move behavior, borrowed views, and concurrent read-only sampling.

10,000 deterministic Bezier cases use independently evaluated Bernstein coordinates at known
parameters. Another 5,000 fixed-seed tracks compare linear sampling against direct weighted
endpoint arithmetic. The executable reports its check count and exits nonzero on failure.
CTest also runs a two-translation-unit standalone header/link test. Tests run with checks enabled
in Release builds; they do not rely on `assert`.

The step-boundary test covers an interior sample that rounds to normalized progress one.
The extrapolation regression proves a mathematically representable result survives cancellation
even when computing the unfused product alone would overflow. Tolerances are absolute, explicit,
and limited to each test corpus; they are not general accuracy guarantees.

## Repeat the full local check

From a clean committed checkout:

```powershell
pwsh -File tools/Verify.ps1 -Generator 'Visual Studio 17 2022' -Toolset v143
```

For Linux, set `CXX=g++-14` or `clang++-18`, then use `-Generator Ninja -Sanitizers`.
Visual Studio 2026 users can pass `-Generator 'Visual Studio 18 2026'` with their stable CMake path.
The optional `-AllowDirty` is for development only and produces `development_verified` evidence.

The script checks the reviewed header SHA, builds with warnings as errors, runs both CTest cases
and the example, installs CMake files, moves that installation, and builds/runs a consumer outside
the checkout. It restores locked packaging dependencies and packs the `.nupkg`. Archive auditing
checks every payload, exact native props/targets names, metadata/source commit, license and icon.
On Windows it installs into a fresh temporary packages directory and compiles/runs a `.vcxproj`
in Debug and Release. MSB8029 may be emitted because the isolated consumer lives under the
temporary directory; that diagnostic does not represent a source/compiler warning.

Evidence is written to ignored `artifacts/verify-<id>/`, including `tests.xml`, exact `.nupkg`,
consumer evidence, and `release-manifest.json`. `artifacts/latest-verification.txt` identifies the
latest run. Local pack success does not imply publication, nor does it prove Linux execution.
Only completed CI jobs supply those platform results.

## Publication

1. Require all Windows/GCC/Clang CI jobs to pass for the selected clean source commit.
2. Download its verified Windows artifact and verify the GitHub artifact digest. Audit the frozen
   `.nupkg` and tie `packages/BenScr.Curves/1.0.0` to that same commit.
3. Publish exactly that `.nupkg`, and attach it with evidence to the matching GitHub release.
   No managed `.snupkg` applies to this header-only package.
4. Run `tools/VerifyNuGetConsumer.ps1 -Published -ExpectedPackage <frozen-package>` to install
   from the real public feed into a new empty directory and compare the installed payloads.
5. Verify NuGet's public archive/signature and the gallery's displayed master icon. Record actual
   indexing/public-consumer status and exact URLs; do not count pending work as complete.

Never overwrite a published version or repack different bytes under an existing release tag.
