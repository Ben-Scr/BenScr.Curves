#include <benscr/curves/curves.hpp>

#include <array>
#include <cstdint>
#include <future>
#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace Curves = BenScr::Curves;
namespace
{
std::size_t checks = 0;
constexpr double kMaximum = std::numeric_limits<double>::max();
constexpr double kInfinity = std::numeric_limits<double>::infinity();
constexpr double kNan = std::numeric_limits<double>::quiet_NaN();

void Check(bool condition, const char *message)
{
    ++checks;
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void Near(double actual, double expected, double tolerance, const char *message)
{
    Check(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, message);
}

template <typename Exception, typename Action> void Throws(Action action)
{
    try
    {
        action();
    }
    catch (const Exception &)
    {
        ++checks;
        return;
    }
    throw std::runtime_error("Expected exception was not thrown.");
}

double Polynomial(double first, double second, double t)
{
    const auto u = 1.0 - t;
    return 3.0 * u * u * t * first + 3.0 * u * t * t * second + t * t * t;
}

void BuiltinVectors()
{
    const std::array<std::pair<Curves::Easing, double>, 7> vectors{
        {{Curves::Easing::Linear, 0.25},
         {Curves::Easing::StepEnd, 0.0},
         {Curves::Easing::QuadraticIn, 0.0625},
         {Curves::Easing::QuadraticOut, 0.4375},
         {Curves::Easing::QuadraticInOut, 0.125},
         {Curves::Easing::SmoothStep, 0.15625},
         {Curves::Easing::SmootherStep, 0.103515625}}};
    for (const auto &[easing, expected] : vectors)
    {
        Check(Curves::Ease(easing, 0.25) == expected, "Builtin known quarter");
        Check(Curves::Ease(easing, -100.0) == 0.0, "Builtin lower clamp");
        Check(Curves::Ease(easing, 100.0) == 1.0, "Builtin upper clamp");
        Check(Curves::Ease(easing, 0.0) == 0.0, "Builtin start");
        Check(Curves::Ease(easing, 1.0) == 1.0, "Builtin end");
    }
    Check(Curves::Ease(Curves::Easing::QuadraticInOut, 0.75) == 0.875, "Quadratic out half");
    Check(Curves::Ease(Curves::Easing::StepEnd, std::nextafter(1.0, 0.0)) == 0.0,
          "Step holds until endpoint");
}

void BuiltinBounds()
{
    for (int kind = 0; kind < 7; ++kind)
    {
        const auto easing = static_cast<Curves::Easing>(kind);
        double previous = 0.0;
        for (int step = 0; step <= 10000; ++step)
        {
            const auto result = Curves::Ease(easing, step / 10000.0);
            Check(result >= 0.0 && result <= 1.0, "Builtin bounded output");
            Check(result >= previous - 2e-15, "Builtin monotonic grid");
            previous = result;
        }
        auto edge = 1.0;
        for (int step = 0; step < 1000; ++step)
        {
            edge = std::nextafter(edge, 0.0);
            const auto result = Curves::Ease(easing, edge);
            Check(result >= 0.0 && result <= 1.0, "Builtin last representable values");
        }
    }
}

void InvalidProgressAndControls()
{
    const Curves::CubicBezier curve(0.25, 0.1, 0.25, 1.0);
    for (auto invalid : {kInfinity, -kInfinity, kNan})
    {
        Throws<std::invalid_argument>([&] { (void)Curves::Ease(Curves::Easing::Linear, invalid); });
        Throws<std::invalid_argument>([&] { (void)curve.Evaluate(invalid); });
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(invalid, 0, 0, 0); });
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(0, invalid, 0, 0); });
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(0, 0, invalid, 0); });
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(0, 0, 0, invalid); });
    }
    for (auto invalid : {-0.001, 1.001})
    {
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(invalid, 0, 0, 0); });
        Throws<std::invalid_argument>([&] { (void)Curves::CubicBezier(0, 0, invalid, 0); });
    }
    const auto invalid = static_cast<Curves::Easing>(99);
    Throws<std::invalid_argument>([&] { (void)Curves::Ease(invalid, 0); });
    Throws<std::invalid_argument>([&] { (void)Curves::TimingFunction(invalid); });
}

void BezierIdentityAndEndpoints()
{
    for (auto x1 : {0.0, 0.3, 1.0})
    {
        for (auto x2 : {0.0, 0.7, 1.0})
        {
            const Curves::CubicBezier identity(x1, x1, x2, x2);
            for (auto p : {0.0, std::numeric_limits<double>::denorm_min(), 1e-200, 0.125, 0.5, 0.9,
                           std::nextafter(1.0, 0.0), 1.0})
            {
                Check(identity.Evaluate(p) == p, "Bezier identity exact");
            }
            const Curves::CubicBezier overshoot(x1, -20.0, x2, 30.0);
            Check(overshoot.X1() == x1 && overshoot.X2() == x2 && overshoot.Y1() == -20.0 &&
                      overshoot.Y2() == 30.0,
                  "Control accessors");
            Check(overshoot.Evaluate(-kMaximum) == 0.0, "Bezier lower clamp");
            Check(overshoot.Evaluate(kMaximum) == 1.0, "Bezier upper clamp");
        }
    }
}

void BezierParameterIsNotTime()
{
    const Curves::CubicBezier cube(0.0, 1.0 / 3.0, 0.0, 2.0 / 3.0);
    Near(cube.Evaluate(0.125), 0.5, 2e-15, "Time mapping cube root");
    const Curves::CubicBezier inverse(1.0, 1.0 / 3.0, 1.0, 2.0 / 3.0);
    Near(inverse.Evaluate(0.875), 0.5, 2e-15, "Time mapping inverse cube root");
    for (auto p : {1e-300, 1e-150, 1e-30, 1e-12, 0.01, 0.5, 0.99})
    {
        const auto expected = std::cbrt(p);
        Near(cube.Evaluate(p), expected, expected * 1e-14, "Tiny progress cube");
    }
    const auto tiny = std::numeric_limits<double>::denorm_min();
    Near(cube.Evaluate(tiny), std::cbrt(tiny), std::cbrt(tiny) * 1e-14, "Subnormal cube");
    // X(t) is approximately 3t near zero; this must not collapse every small input to zero.
    Check(inverse.Evaluate(1e-200) > 0.0, "Inverse small nonzero progress");
}

void BezierStationaryMidpoint()
{
    const Curves::CubicBezier curve(1.0, 1.0 / 3.0, 0.0, 2.0 / 3.0);
    Check(curve.Evaluate(0.5) == 0.5, "Stationary midpoint exact");
    for (auto p : {std::nextafter(0.5, 0.0), std::nextafter(0.5, 1.0), 0.499999, 0.500001})
    {
        const auto expected = 0.5 + std::cbrt((p - 0.5) / 4.0);
        Near(curve.Evaluate(p), expected, 5e-16, "Stationary midpoint adjacent input");
    }
}

void BezierKnownPolynomials()
{
    const Curves::CubicBezier smooth(1.0 / 3.0, 0.0, 2.0 / 3.0, 1.0);
    Near(smooth.Evaluate(0.25), 0.15625, 2e-15, "Bezier independent polynomial");
    Near(smooth.Evaluate(0.75), 0.84375, 2e-15, "Bezier independent polynomial upper");
    const Curves::CubicBezier ease(0.25, 0.1, 0.25, 1.0);
    Near(ease.Evaluate(0.25), 0.4085105913553959, 2e-14, "CSS ease quarter reference");
    Near(ease.Evaluate(0.5), 0.8024033875848570, 2e-14, "CSS ease half reference");
    Near(ease.Evaluate(0.75), 0.9604589783489740, 2e-14, "CSS ease three quarters");
}

void BezierOvershootAndExtremeControls()
{
    const Curves::CubicBezier above(1.0 / 3.0, 5.0, 2.0 / 3.0, 5.0);
    Near(above.Evaluate(0.5), 3.875, 2e-14, "Overshoot is preserved");
    const Curves::CubicBezier below(1.0 / 3.0, -5.0, 2.0 / 3.0, -5.0);
    Check(below.Evaluate(0.5) < 0.0, "Undershoot is preserved");
    for (auto y1 : {-kMaximum, kMaximum})
    {
        for (auto y2 : {-kMaximum, kMaximum})
        {
            const Curves::CubicBezier extreme(0.25, y1, 0.75, y2);
            for (int i = 0; i <= 100; ++i)
            {
                Check(std::isfinite(extreme.Evaluate(i / 100.0)), "Extreme convex hull finite");
            }
        }
    }
}

void BezierIndependentReferenceSweep()
{
    std::mt19937_64 random(0xc0a7e5);
    for (int i = 0; i < 10000; ++i)
    {
        // Direct Bernstein evaluation at a known parameter avoids inverse-solver roundtrips.
        const auto x1 = static_cast<double>(random() % 1001) / 1000.0;
        const auto x2 = static_cast<double>(random() % 1001) / 1000.0;
        const auto y1 = static_cast<double>(random() % 4001) / 1000.0 - 2.0;
        const auto y2 = static_cast<double>(random() % 4001) / 1000.0 - 2.0;
        const auto t = static_cast<double>(1 + random() % 9999) / 10000.0;
        const auto input = Polynomial(x1, x2, t);
        const auto expected = Polynomial(y1, y2, t);
        const Curves::CubicBezier curve(x1, y1, x2, y2);
        Near(curve.Evaluate(input), expected, 2e-10, "Independent Bezier reference");
    }
}

void TimingValueSemantics()
{
    Check(Curves::TimingFunction().Evaluate(0.2) == 0.2, "Default timing linear");
    Curves::TimingFunction value(Curves::Easing::QuadraticIn);
    auto copy = value;
    value = Curves::TimingFunction(Curves::CubicBezier(0, 0, 1, 1));
    Check(copy.Evaluate(0.5) == 0.25 && value.Evaluate(0.5) == 0.5, "Timing copied value");
    Throws<std::invalid_argument>([&] { (void)copy.Evaluate(kNan); });
}

void EmptyAndSingleTracks()
{
    const Curves::KeyframeTrack empty;
    Check(empty.Empty() && empty.Size() == 0 && empty.Keyframes().empty(), "Default empty");
    Throws<std::logic_error>([&] { (void)empty.Sample(0); });
    Throws<std::invalid_argument>([&] { (void)empty.Sample(kNan); });
    const Curves::KeyframeTrack explicitEmpty(std::span<const Curves::Keyframe>{});
    Check(explicitEmpty.Empty(), "Explicit empty");
    const std::array<Curves::Keyframe, 1> keys{{{5, -7, {}}}};
    const Curves::KeyframeTrack single(keys);
    for (auto t : {-kMaximum, -1.0, 5.0, 10.0, kMaximum})
    {
        Check(single.Sample(t) == -7.0, "One key clamps all finite times");
    }
    for (auto t : {kNan, kInfinity, -kInfinity})
    {
        Throws<std::invalid_argument>([&] { (void)single.Sample(t); });
    }
}

void TrackValidation()
{
    for (auto invalid : {kNan, kInfinity, -kInfinity})
    {
        const std::array<Curves::Keyframe, 1> badTime{{{invalid, 0, {}}}};
        const std::array<Curves::Keyframe, 1> badValue{{{0, invalid, {}}}};
        Throws<std::invalid_argument>([&] { (void)Curves::KeyframeTrack(badTime); });
        Throws<std::invalid_argument>([&] { (void)Curves::KeyframeTrack(badValue); });
    }
    for (auto second : {0.0, -1.0, -0.0})
    {
        const std::array<Curves::Keyframe, 2> keys{{{0, 1, {}}, {second, 2, {}}}};
        Throws<std::invalid_argument>([&] { (void)Curves::KeyframeTrack(keys); });
    }
    std::vector<Curves::Keyframe> excessive(Curves::KeyframeTrack::kMaxKeyframes + 1);
    Throws<std::length_error>([&] { (void)Curves::KeyframeTrack(excessive); });
}

void TrackSegmentsAndKeys()
{
    const std::array<Curves::Keyframe, 4> keys{
        {{-2, 10, Curves::TimingFunction(Curves::Easing::StepEnd)},
         {0, 20, Curves::TimingFunction(Curves::Easing::QuadraticIn)},
         {4, 100, Curves::TimingFunction(Curves::CubicBezier(0, 1.0 / 3, 0, 2.0 / 3))},
         {12, 200, {}}}};
    const Curves::KeyframeTrack track(keys);
    Check(track.Size() == 4 && !track.Empty(), "Track size");
    for (const auto &key : keys)
    {
        Check(track.Sample(key.Time) == key.Value, "Exact key values");
    }
    Check(track.Sample(-100) == 10 && track.Sample(100) == 200, "Track endpoint clamp");
    Check(track.Sample(-1) == 10, "Step segment");
    Check(track.Sample(2) == 40, "Quadratic outgoing segment");
    Near(track.Sample(5), 150, 1e-12, "Bezier outgoing segment");
    Check(track.Sample(std::nextafter(0.0, -1.0)) == 10, "Step just before next key");
}

void ExtremeTimesAndValues()
{
    const std::array<Curves::Keyframe, 2> keys{
        {{-kMaximum, -kMaximum, {}}, {kMaximum, kMaximum, {}}}};
    const Curves::KeyframeTrack track(keys);
    Check(track.Sample(0) == 0, "Extreme opposite endpoints midpoint");
    Near(track.Sample(-kMaximum / 2) / kMaximum, -0.5, 1e-15, "Extreme negative quarter");
    Near(track.Sample(kMaximum / 2) / kMaximum, 0.5, 1e-15, "Extreme positive quarter");
    const auto small = std::numeric_limits<double>::denorm_min();
    const std::array<Curves::Keyframe, 2> smallKeys{{{0, 0, {}}, {small * 2, 10, {}}}};
    Check(Curves::KeyframeTrack(smallKeys).Sample(small) == 5, "Subnormal time interval");
    const std::array<Curves::Keyframe, 2> adjacent{{{1, 2, {}}, {std::nextafter(1.0, 2.0), 4, {}}}};
    const Curves::KeyframeTrack narrow(adjacent);
    Check(narrow.Sample(1) == 2 && narrow.Sample(adjacent[1].Time) == 4, "Adjacent key times");
}

void TrackOvershootOverflow()
{
    const Curves::TimingFunction overshoot(Curves::CubicBezier(1.0 / 3, 5, 2.0 / 3, 5));
    const std::array<Curves::Keyframe, 2> finite{{{0, kMaximum, overshoot}, {1, kMaximum / 2, {}}}};
    const Curves::KeyframeTrack track(finite);
    Near(track.Sample(0.5) / kMaximum, -0.9375, 2e-14,
         "Representable extrapolation must not overflow its product");
    const std::array<Curves::Keyframe, 2> overflow{{{0, kMaximum, overshoot}, {1, -kMaximum, {}}}};
    const Curves::KeyframeTrack impossible(overflow);
    Throws<std::overflow_error>([&] { (void)impossible.Sample(0.5); });
    Check(impossible.Sample(0) == kMaximum && impossible.Sample(1) == -kMaximum,
          "Overflow failure does not mutate track");
}

void TrackOwnershipAndMoves()
{
    std::vector<Curves::Keyframe> source{{0, 10, {}}, {1, 20, {}}};
    Curves::KeyframeTrack track(source);
    source[0].Value = 1000;
    source.clear();
    Check(track.Sample(0.5) == 15, "Track copies input");
    auto copied = track;
    Curves::KeyframeTrack moved(std::move(track));
    Check(track.Empty() && moved.Sample(0.5) == 15 && copied.Sample(0.5) == 15,
          "Copy/move constructor");
    Throws<std::logic_error>([&] { (void)track.Sample(0); });
    track = copied;
    copied = std::move(moved);
    Check(moved.Empty() && track.Sample(0.5) == 15 && copied.Sample(0.5) == 15, "Assignments");
    auto *alias = &copied;
    copied = std::move(*alias);
    Check(copied.Sample(0.5) == 15, "Self move remains valid");
    Check(track.Keyframes()[0].Value == 10, "Read only owned view");
}

void TrackReferenceSweep()
{
    std::mt19937_64 random(0x1e7f00);
    for (int iteration = 0; iteration < 5000; ++iteration)
    {
        std::vector<Curves::Keyframe> keys;
        double time = -20.0;
        const auto count = 2 + random() % 10;
        for (std::uint64_t index = 0; index < count; ++index)
        {
            time += 1 + static_cast<double>(random() % 10);
            keys.push_back({time, static_cast<double>(random() % 2001) - 1000.0, {}});
        }
        const Curves::KeyframeTrack track(keys);
        const auto segment = static_cast<std::size_t>(random() % (keys.size() - 1));
        const auto fraction = static_cast<double>(random() % 101) / 100;
        const auto sampleTime =
            keys[segment].Time + fraction * (keys[segment + 1].Time - keys[segment].Time);
        const auto u =
            (sampleTime - keys[segment].Time) / (keys[segment + 1].Time - keys[segment].Time);
        const auto expected = (1.0 - u) * keys[segment].Value + u * keys[segment + 1].Value;
        Near(track.Sample(sampleTime), expected, 1e-10, "Independent linear track reference");
    }
}

void ConcurrentReadOnlySampling()
{
    const std::array<Curves::Keyframe, 2> keys{
        {{0, 0, Curves::TimingFunction(Curves::Easing::QuadraticIn)}, {10, 100, {}}}};
    const Curves::KeyframeTrack track(keys);
    std::vector<std::future<bool>> runs;
    for (int worker = 0; worker < 4; ++worker)
    {
        runs.push_back(std::async(std::launch::async,
                                  [&track]
                                  {
                                      for (int i = 0; i < 1000; ++i)
                                      {
                                          const auto t = i / 100.0;
                                          if (std::abs(track.Sample(t) - t * t) > 1e-12)
                                          {
                                              return false;
                                          }
                                      }
                                      return true;
                                  }));
    }
    for (auto &run : runs)
    {
        Check(run.get(), "Concurrent const sampling");
    }
}
} // namespace

int main()
{
    const std::array<std::pair<const char *, void (*)()>, 18> tests{
        {{"builtin vectors", BuiltinVectors},
         {"builtin bounds", BuiltinBounds},
         {"invalid progress and controls", InvalidProgressAndControls},
         {"Bezier identity and endpoints", BezierIdentityAndEndpoints},
         {"Bezier time mapping", BezierParameterIsNotTime},
         {"stationary midpoint", BezierStationaryMidpoint},
         {"known polynomials", BezierKnownPolynomials},
         {"overshoot and extreme controls", BezierOvershootAndExtremeControls},
         {"independent Bezier reference", BezierIndependentReferenceSweep},
         {"timing values", TimingValueSemantics},
         {"empty and single tracks", EmptyAndSingleTracks},
         {"track validation", TrackValidation},
         {"segments and keys", TrackSegmentsAndKeys},
         {"extreme times and values", ExtremeTimesAndValues},
         {"track overshoot overflow", TrackOvershootOverflow},
         {"ownership and moves", TrackOwnershipAndMoves},
         {"track reference", TrackReferenceSweep},
         {"concurrent reads", ConcurrentReadOnlySampling}}};
    for (const auto &[name, test] : tests)
    {
        try
        {
            test();
        }
        catch (const std::exception &error)
        {
            std::cerr << name << ": " << error.what() << '\n';
            return 1;
        }
    }
    std::cout << tests.size() << " contract cases passed; " << checks
              << " checks; 15000 fixed-seed reference inputs.\n";
}
