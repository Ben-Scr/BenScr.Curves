#ifndef BENSCR_CURVES_CURVES_HPP
#define BENSCR_CURVES_CURVES_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace BenScr::Curves
{
/// Built-in normalized timing functions. StepEnd holds zero until progress reaches one.
enum class Easing
{
    Linear,
    StepEnd,
    QuadraticIn,
    QuadraticOut,
    QuadraticInOut,
    SmoothStep,
    SmootherStep
};

namespace Detail
{
inline double ClampProgress(double progress)
{
    if (!std::isfinite(progress))
    {
        throw std::invalid_argument("Progress must be finite.");
    }
    return std::clamp(progress, 0.0, 1.0);
}

inline void ValidateEasing(Easing easing)
{
    switch (easing)
    {
    case Easing::Linear:
    case Easing::StepEnd:
    case Easing::QuadraticIn:
    case Easing::QuadraticOut:
    case Easing::QuadraticInOut:
    case Easing::SmoothStep:
    case Easing::SmootherStep:
        return;
    }
    throw std::invalid_argument("Invalid easing value.");
}

inline double BezierCoordinate(double first, double second, double parameter) noexcept
{
    // Every interpolation is between finite endpoints, even for extreme control values.
    const auto left = std::lerp(0.0, first, parameter);
    const auto center = std::lerp(first, second, parameter);
    const auto right = std::lerp(second, 1.0, parameter);
    return std::lerp(std::lerp(left, center, parameter), std::lerp(center, right, parameter),
                     parameter);
}
} // namespace Detail

/// Evaluates a built-in easing with finite progress clamped to [0,1].
/// Invalid enum values and non-finite progress throw std::invalid_argument.
[[nodiscard]] inline double Ease(Easing easing, double progress)
{
    Detail::ValidateEasing(easing);
    const auto t = Detail::ClampProgress(progress);
    if (t == 0.0 || t == 1.0)
    {
        return t;
    }
    switch (easing)
    {
    case Easing::Linear:
        return t;
    case Easing::StepEnd:
        return 0.0;
    case Easing::QuadraticIn:
        return t * t;
    case Easing::QuadraticOut:
        return t * (2.0 - t);
    case Easing::QuadraticInOut:
        return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t);
    case Easing::SmoothStep:
        return std::clamp(t * t * (3.0 - 2.0 * t), 0.0, 1.0);
    case Easing::SmootherStep:
        return std::clamp(t * t * t * (t * (6.0 * t - 15.0) + 10.0), 0.0, 1.0);
    }
    throw std::invalid_argument("Invalid easing value.");
}

/// Time-mapped cubic Bezier with fixed endpoints (0,0) and (1,1).
/// X controls must lie in [0,1]; Y controls may be any finite doubles, including overshoot.
/// Pure value type, allocation-free and safe for concurrent reads.
class CubicBezier
{
  public:
    CubicBezier(double x1, double y1, double x2, double y2) : x1(x1), y1(y1), x2(x2), y2(y2)
    {
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2) ||
            x1 < 0.0 || x1 > 1.0 || x2 < 0.0 || x2 > 1.0)
        {
            throw std::invalid_argument("Bezier controls must be finite, with X in [0,1].");
        }
    }

    [[nodiscard]] double X1() const noexcept
    {
        return x1;
    }
    [[nodiscard]] double Y1() const noexcept
    {
        return y1;
    }
    [[nodiscard]] double X2() const noexcept
    {
        return x2;
    }
    [[nodiscard]] double Y2() const noexcept
    {
        return y2;
    }

    /// Solves X(t)=progress before evaluating Y(t); this is not direct parameter sampling.
    /// Finite progress is clamped to [0,1]. Non-finite progress throws invalid_argument.
    /// Endpoints are exact. Interior results are double-precision approximations; flat X
    /// derivatives amplify rounding. Evaluation uses at most 1100 bounded bisection steps.
    [[nodiscard]] double Evaluate(double progress) const
    {
        const auto input = Detail::ClampProgress(progress);
        if (input == 0.0 || input == 1.0)
        {
            return input;
        }
        if (x1 == y1 && x2 == y2)
        {
            return input;
        }
        double parameter;
        if (x1 == 0.0 && x2 == 0.0)
        {
            parameter = std::cbrt(input);
        }
        else if (x1 == 1.0 && x2 == 1.0)
        {
            parameter = -std::expm1(std::log1p(-input) / 3.0);
        }
        else if (x1 == 1.0 && x2 == 0.0 && input >= 0.25 && input <= 0.75)
        {
            // X(t)=1/2+4(t-1/2)^3; retain precision at its stationary midpoint.
            parameter = 0.5 + std::cbrt((input - 0.5) * 0.25);
        }
        else
        {
            double lower = 0.0;
            double upper = 1.0;
            parameter = 0.5;
            for (int iteration = 0; iteration < 1100; ++iteration)
            {
                parameter = lower + (upper - lower) * 0.5;
                if (parameter == lower || parameter == upper)
                {
                    break;
                }
                const auto x = Detail::BezierCoordinate(x1, x2, parameter);
                if (x == input)
                {
                    break;
                }
                if (x < input)
                {
                    lower = parameter;
                }
                else
                {
                    upper = parameter;
                }
            }
        }
        return Detail::BezierCoordinate(y1, y2, parameter);
    }

  private:
    double x1;
    double y1;
    double x2;
    double y2;
};

/// Owns either a built-in easing or a cubic Bezier. Default construction is linear.
class TimingFunction
{
  public:
    TimingFunction() noexcept = default;

    explicit TimingFunction(Easing easing) : function(easing)
    {
        Detail::ValidateEasing(easing);
    }

    explicit TimingFunction(CubicBezier curve) noexcept : function(curve) {}

    [[nodiscard]] double Evaluate(double progress) const
    {
        if (const auto *easing = std::get_if<Easing>(&function))
        {
            return Ease(*easing, progress);
        }
        return std::get<CubicBezier>(function).Evaluate(progress);
    }

  private:
    std::variant<Easing, CubicBezier> function = Easing::Linear;
};

/// Time is caller-defined (e.g. seconds). Outgoing shapes the segment to the next key.
/// The final key's Outgoing has no effect. Time and Value must be finite in a track.
struct Keyframe
{
    double Time = 0.0;
    double Value = 0.0;
    TimingFunction Outgoing;
};

/// Owns a validated copy of strictly time-ordered keys, without a sampling cache.
/// Concurrent const reads are safe. Assignment/moving require external synchronization.
class KeyframeTrack
{
  public:
    static constexpr std::size_t kMaxKeyframes = 1'000'000;

    KeyframeTrack() = default;

    /// Empty tracks are valid. Invalid keys/order throw invalid_argument; too many keys
    /// throw length_error. Construction allocates and may throw bad_alloc.
    explicit KeyframeTrack(std::span<const Keyframe> source)
    {
        if (source.size() > kMaxKeyframes)
        {
            throw std::length_error("Keyframe count exceeds the limit.");
        }
        for (std::size_t index = 0; index < source.size(); ++index)
        {
            if (!std::isfinite(source[index].Time) || !std::isfinite(source[index].Value) ||
                (index > 0 && source[index].Time <= source[index - 1].Time))
            {
                throw std::invalid_argument(
                    "Keys need finite values and strictly increasing times.");
            }
        }
        keys.assign(source.begin(), source.end());
    }

    KeyframeTrack(const KeyframeTrack &) = default;
    KeyframeTrack &operator=(const KeyframeTrack &) = default;

    KeyframeTrack(KeyframeTrack &&other) noexcept : keys(std::move(other.keys))
    {
        other.keys.clear();
    }

    KeyframeTrack &operator=(KeyframeTrack &&other) noexcept
    {
        if (this != &other)
        {
            keys = std::move(other.keys);
            other.keys.clear();
        }
        return *this;
    }

    [[nodiscard]] bool Empty() const noexcept
    {
        return keys.empty();
    }
    [[nodiscard]] std::size_t Size() const noexcept
    {
        return keys.size();
    }

    /// Borrowed read-only view, invalidated by assignment, moving or destruction of this track.
    [[nodiscard]] std::span<const Keyframe> Keyframes() const noexcept
    {
        return keys;
    }

    /// Samples without allocation. Finite times outside the track are clamped to endpoint values.
    /// Non-finite time throws invalid_argument, an empty track throws logic_error, and a
    /// non-representable overshooting result throws overflow_error. Exact key times return keys.
    [[nodiscard]] double Sample(double time) const
    {
        if (!std::isfinite(time))
        {
            throw std::invalid_argument("Sample time must be finite.");
        }
        if (keys.empty())
        {
            throw std::logic_error("Cannot sample an empty track.");
        }
        if (time <= keys.front().Time)
        {
            return keys.front().Value;
        }
        if (time >= keys.back().Time)
        {
            return keys.back().Value;
        }
        const auto right =
            std::upper_bound(keys.begin(), keys.end(), time,
                             [](double value, const Keyframe &key) { return value < key.Time; });
        const auto &left = *(right - 1);
        if (time == left.Time)
        {
            return left.Value;
        }
        const auto duration = right->Time - left.Time;
        const auto fraction = std::isfinite(duration) ? (time - left.Time) / duration
                                                      : (time * 0.5 - left.Time * 0.5) /
                                                            (right->Time * 0.5 - left.Time * 0.5);
        // This is a strictly interior time: a rounded ratio must not end a step early.
        const auto progress = std::min(fraction, std::nextafter(1.0, 0.0));
        const auto eased = left.Outgoing.Evaluate(progress);
        double result;
        if (eased >= 0.0 && eased <= 1.0)
        {
            result = std::lerp(left.Value, right->Value, eased);
        }
        else
        {
            const auto difference = right->Value - left.Value;
            result =
                std::isfinite(difference)
                    ? std::fma(eased, difference, left.Value)
                    : std::fma(eased, right->Value * 0.5 - left.Value * 0.5, left.Value * 0.5) *
                          2.0;
        }
        if (!std::isfinite(result))
        {
            throw std::overflow_error("The interpolated value exceeds the finite double range.");
        }
        return result;
    }

  private:
    std::vector<Keyframe> keys;
};
} // namespace BenScr::Curves

#endif
