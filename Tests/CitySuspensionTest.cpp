#include "CitySuspension.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using CitySuspension::Force;
    assert(Force(0, 0) * 4.f == 1200.f * 980.f);
    assert(Force(10, 0) > Force(0, 0));
    assert(Force(0, -30) > Force(0, 0));
    assert(Force(0, 30) < Force(0, 0));
    assert(Force(-100, 0) >= 0.f);
    assert(Force(0, 1000) == 0.f);
    assert(Force(1000, -1000) <= 1200.f * 980.f * .5f);
    assert(Force(0, 0, 1600.f) * 4.f == 1600.f * 980.f);
    std::cout << "PASS: static weight support, compression, damping and bounded force\n";
    // Forces are held over each game frame, as with game-thread AddForce and
    // physics substeps. Test that a step up/down settles instead of gaining energy.
    for (int Fps : {10, 15, 30, 60, 120})
    {
        const float Dt = 1.f / Fps;
        float Height = 82.f, Velocity = 0.f;
        for (int Frame = 0; Frame < Fps * 10; ++Frame)
        {
            const float Ground = Frame >= Fps * 2 && Frame < Fps * 4 ? 15.f : 0.f;
            const float Lift = Force(62.f + Ground - Height, Velocity, 1200.f, Dt) * 4.f;
            const float Acceleration = Lift / 1200.f - 980.f;
            for (int Step = 0; Step < 12; ++Step)
            {
                const float SubDt = Dt / 12.f;
                Velocity += Acceleration * SubDt;
                Height += Velocity * SubDt;
            }
            assert(std::isfinite(Height) && Height > 40.f && Height < 105.f);
        }
        assert(std::abs(Height - 62.f) < .2f && std::abs(Velocity) < .2f);
        std::cout << "PASS: 15 cm bump settles without runaway bounce at " << Fps << " FPS\n";
        // Pitch/roll have lower contact-effective mass than vertical heave.
        for (float ModeMass : {75.f, 150.f, 300.f})
        {
            float Deflection = 15.f, ModeVelocity = 0.f;
            for (int Frame = 0; Frame < Fps * 12; ++Frame)
            {
                const float Acceleration = (Force(-Deflection, ModeVelocity, 1200.f, Dt) - 1200.f * 980.f / 4.f) / ModeMass;
                for (int Step = 0; Step < 12; ++Step)
                {
                    ModeVelocity += Acceleration * (Dt / 12.f);
                    Deflection += ModeVelocity * (Dt / 12.f);
                }
                assert(std::abs(Deflection) < 20.f && std::isfinite(Deflection));
            }
            assert(std::abs(Deflection) < .2f && std::abs(ModeVelocity) < .2f);
        }
    }
}
