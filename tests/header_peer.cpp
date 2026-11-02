#include <benscr/curves/curves.hpp>

double PeerSample()
{
    return BenScr::Curves::CubicBezier(0.0, 0.0, 1.0, 1.0).Evaluate(0.5);
}
