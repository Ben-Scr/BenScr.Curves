#include <benscr/curves/curves.hpp>

double PeerSample();

int main()
{
    return BenScr::Curves::Ease(BenScr::Curves::Easing::Linear, 0.5) == PeerSample() ? 0 : 1;
}
