#include <benscr/curves/curves.hpp>

#include <array>
#include <cmath>
#include <iostream>

namespace Curves = BenScr::Curves;

int main()
{
    const std::array<Curves::Keyframe, 3> keys{
        {{0.0, 10.0, Curves::TimingFunction(Curves::Easing::QuadraticIn)},
         {2.0, 30.0, Curves::TimingFunction(Curves::Easing::StepEnd)},
         {4.0, 50.0, {}}}};
    const Curves::KeyframeTrack track(keys);
    const Curves::CubicBezier ease(0.25, 0.1, 0.25, 1.0);
    if (track.Sample(1.0) != 15.0 || track.Sample(3.0) != 30.0 || track.Sample(4.0) != 50.0 ||
        track.Sample(-1.0) != 10.0 || std::abs(ease.Evaluate(0.5) - 0.802403387584857) > 1e-13)
    {
        return 1;
    }
    auto copied = track;
    auto moved = std::move(copied);
    if (!copied.Empty() || moved.Sample(1.0) != 15.0)
    {
        return 2;
    }
    bool rejected = false;
    try
    {
        (void)Curves::CubicBezier(-0.1, 0.0, 1.0, 1.0);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    if (!rejected)
    {
        return 3;
    }
    std::cout << "Curves verified.\n";
}
