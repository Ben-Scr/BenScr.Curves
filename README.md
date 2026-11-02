# BenScr.Curves

C++20 easing functions, time-mapped cubic Bezier curves, and owning scalar keyframe tracks.
Header-only, with no runtime package dependencies or engine types.

Created by Ben Schneider — https://github.com/Ben-Scr

```cpp
#include <benscr/curves/curves.hpp>
#include <array>

namespace Curves = BenScr::Curves;

const std::array<Curves::Keyframe, 3> keys{{
    {0.0, 10.0, Curves::TimingFunction(Curves::Easing::QuadraticIn)},
    {2.0, 30.0, Curves::TimingFunction(Curves::Easing::StepEnd)},
    {4.0, 50.0, {}}}};
const Curves::KeyframeTrack track(keys);

// Time units are chosen by the application, for example seconds.
const double halfway = track.Sample(1.0); // 15: quadratic progress through the first segment
const double held = track.Sample(3.0);    // 30: step holds until the next key
const double end = track.Sample(4.0);     // 50: exact key value

const Curves::CubicBezier ease(0.25, 0.1, 0.25, 1.0);
const double progress = ease.Evaluate(0.5); // approximately 0.8024033876
```

The complete executable example is [examples/curves.cpp](examples/curves.cpp).

## Native NuGet installation

Install **BenScr.Curves 1.0.0** into a native Visual Studio C++ project using `packages.config`:

```xml
<packages>
  <package id="BenScr.Curves" version="1.0.0" targetFramework="native" />
</packages>
```

Set the project's C++ language standard to C++20. Import
`build/native/BenScr.Curves.props` after `Microsoft.Cpp.props`, and
`build/native/BenScr.Curves.targets` after `Microsoft.Cpp.targets`, as Visual Studio's native
NuGet integration does. The props adds the include directory; the targets validates the header.
`tools/VerifyNuGetConsumer.ps1` contains a complete isolated `.vcxproj` example.

The tested native NuGet path is Windows x64, MSVC v143, Debug and Release. Consumers compile the
header with their own compiler and CRT settings; no `.lib`, DLL, managed assembly, native ABI, or
separate symbol archive is distributed. Other architectures are not claimed as tested. This is a
direct C++ package, not a C# binding. A `.csproj` under `package/` only drives NuGet packaging.

## CMake installation

With CMake 3.28 or newer and a supported C++20 compiler:

```shell
cmake -S . -B out/build -DBENSCR_CURVES_BUILD_TESTS=OFF -DBENSCR_CURVES_BUILD_EXAMPLES=OFF
cmake --install out/build --prefix /path/to/install
```

Then configure your consumer with `CMAKE_PREFIX_PATH=/path/to/install`:

```cmake
find_package(BenScrCurves 1 CONFIG REQUIRED)
target_link_libraries(MyApplication PRIVATE BenScr::Curves)
```

The exported CMake target works after moving the installation. `add_subdirectory` also exposes
`BenScr::Curves`; examples and tests are off by default when included as a subproject. Warnings
and sanitizer options apply only to this repository's own executable targets. CMake installation
and native NuGet integration are independent tested consumption paths.

## Easing and Bezier contracts

`Ease(Easing, progress)` provides `Linear`, `StepEnd`, `QuadraticIn`, `QuadraticOut`,
`QuadraticInOut`, `SmoothStep`, and `SmootherStep`. Progress must be finite and is clamped to
`[0,1]`; the output of built-ins is also bounded to `[0,1]`. Endpoints return exactly zero and one.
`StepEnd` returns zero for every progress below one, then one at the endpoint. Invalid enum values,
NaN, and infinities throw `std::invalid_argument`.

`CubicBezier(x1, y1, x2, y2)` fixes its endpoints at `(0,0)` and `(1,1)`. Both X controls must be in
`[0,1]`; both Y controls may be any finite doubles. This parameterization follows the
[W3C description of cubic Bezier easing](https://www.w3.org/TR/css-easing-1/#cubic-bezier-easing-functions):
the input is the curve's **X coordinate**, and the output is its corresponding Y coordinate.
Evaluation solves for the curve parameter before evaluating Y. Y can overshoot below zero or
above one; it is not clamped. Input progress is finite and clamped here, so this API does not
implement CSS's tangent extrapolation outside the interval or its other timing-function syntax.

Evaluation uses double precision and stable convex interpolations. Special cubic cases avoid
loss of significance near a stationary midpoint or tiny endpoint inputs. General inversion uses
a bounded bisection, stopping at an exact rounded X sample or adjacent representable parameter
bounds, with an upper limit of 1100 iterations. Ordinary interior inputs typically need far fewer.
There is no fixed absolute or relative accuracy guarantee: nearly stationary X derivatives,
subnormal values, and extremely large Y controls can amplify input and inversion rounding.
Cross-platform bit identity is not promised. Use ordinary bounded control values when animation
precision matters; these are floating-point timing curves, not exact arithmetic or interval bounds.
Fast-math, flush-to-zero modes, and non-default floating-point rounding are outside the tested contract.

`TimingFunction` owns either a built-in easing or a `CubicBezier`. Default construction is linear;
its explicit constructors copy the selected value. Evaluations do not allocate on success.

## Keyframe contracts

`KeyframeTrack` copies up to `kMaxKeyframes` (1,000,000) keys from a span. Key times and values must
be finite; times must be strictly increasing. Duplicate times, including `-0.0` and `+0.0`, are
rejected. The constructor never sorts or changes caller data. Each key's `Outgoing` timing function
shapes the segment leading to the next key; the final key's timing is unused.

`Sample(time)` uses binary search and does not allocate on success. Times outside the key range
are clamped to the first or last value. Exact key times return the stored value. A step segment
holds its left value until the actual next key time, including when a normalized interior time
would round to one. A one-key track returns that key's value for every finite time.

Empty tracks are valid and cannot be sampled. Default construction and moved-from tracks are
empty; `Empty()` and `Size()` inspect this state. Copies own independent key arrays. Move
construction and move assignment leave the source empty; self-move preserves the value.
`Keyframes()` is a borrowed read-only view, valid until the owning track is assigned, moved from,
or destroyed. Do not retain views across these operations.

Extreme finite time endpoints are normalized without overflowing their difference. Ordinary
interpolation remains between finite endpoint values. Bezier overshoot may produce an
unrepresentable value; `Sample` then throws `std::overflow_error` without changing the track.
Large opposite-sign values and cancellation in extrapolation are handled with scaled differences
and fused operations where needed. Floating-point rounding still applies.

| Failure | Exception |
| --- | --- |
| Non-finite sample time, invalid controls/easing, non-finite or unordered keys | `std::invalid_argument` |
| Key count above the documented limit | `std::length_error` |
| Sampling an empty track | `std::logic_error` |
| Overshooting result outside finite double range | `std::overflow_error` |
| Allocation failure while constructing or copying keys | `std::bad_alloc` |

Validation of sample time precedes the empty-track check. Evaluation never modifies a track.
All types support concurrent const reads without shared caches. Mutating a source collection
during construction, or assigning/moving an object during reads, requires external synchronization.
No repeat, ping-pong, callback, multidimensional spline, or automatic clock scheduling is supplied.

## Build and verification

`tools/Verify.ps1` builds tests and the example, checks the public-header baseline, runs CTest,
moves a CMake installation before building a separate consumer, packs and audits the exact
archive, and on Windows restores that archive into fresh Debug/Release native NuGet consumers.
Use a clean committed checkout, PowerShell 7, .NET SDK 10.0.303, and a supported C++ toolchain:

```powershell
pwsh -File tools/Verify.ps1 -Generator 'Visual Studio 17 2022' -Toolset v143
```

CI covers Windows/MSVC and Ubuntu 24.04 with GCC 14 and Clang 18, including ASan/UBSan on Linux.
See [verification and release steps](docs/VERIFICATION.md), [API inventory](docs/API.md),
[migration notes](docs/MIGRATION.md), and [provenance](docs/PROVENANCE.md).
The unchanged master icon is embedded in the package. The original custom attribution license
is included as [LICENSE.txt](LICENSE.txt).
