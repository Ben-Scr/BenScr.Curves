# API inventory — 1.0.0

Created by Ben Schneider — https://github.com/Ben-Scr

Header: `<benscr/curves/curves.hpp>`. Namespace: `BenScr::Curves`.
`Detail` is an implementation namespace, not supported public API.

| API | Contract |
| --- | --- |
| `enum class Easing` | Linear, StepEnd, QuadraticIn, QuadraticOut, QuadraticInOut, SmoothStep, SmootherStep |
| `double Ease(Easing, double progress)` | Finite clamped progress; bounded normalized result; invalid arguments throw |
| `CubicBezier(double x1, double y1, double x2, double y2)` | Finite controls; X coordinates in [0,1] |
| `CubicBezier::X1/Y1/X2/Y2() const noexcept` | Original control coordinates |
| `CubicBezier::Evaluate(double progress) const` | Solve X(t)=progress, return Y(t); finite clamped input, overshoot preserved |
| `TimingFunction()` | Linear value |
| `explicit TimingFunction(Easing)` | Validate and own selected easing |
| `explicit TimingFunction(CubicBezier) noexcept` | Own selected curve |
| `TimingFunction::Evaluate(double progress) const` | Evaluate the owned function with the same progress contract |
| `struct Keyframe` | Public `double Time=0`, `double Value=0`, `TimingFunction Outgoing` |
| `KeyframeTrack::kMaxKeyframes` | `static constexpr size_t`, 1,000,000 |
| `KeyframeTrack()` | Empty track |
| `explicit KeyframeTrack(span<const Keyframe>)` | Validate and copy; empty input allowed |
| KeyframeTrack copy construction/assignment | Independent owned key array |
| KeyframeTrack move construction/assignment | `noexcept`; source empty afterward; self-move preserves value |
| `Empty() const noexcept`, `Size() const noexcept` | Track state |
| `Keyframes() const noexcept` | Borrowed `span<const Keyframe>` invalidated by assignment/move/destruction |
| `Sample(double time) const` | Finite time, endpoint clamp, exact keys; throws for empty or overflowing result |

All methods and free functions use PascalCase; fields and constants follow their stated visibility
rules. Value types have their regular compiler-provided copy/move operations where not explicitly
listed. The [README](../README.md) specifies numeric, error, lifetime and concurrency behavior.
`public-header-baseline.json` pins the normalized full public header so any interface or inline
implementation change requires deliberate baseline review. The standalone-header test links two
translation units to catch missing includes and multiple-definition mistakes.
