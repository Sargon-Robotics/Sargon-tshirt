#pragma once
#include <frc/Ultrasonic.h>
// echo 0, trigger 1

namespace Distance {
    static std::optional<frc::Ultrasonic> usonic;

    void init() {
        usonic = frc::Ultrasonic{0, 1};
    }

    units::meter_t getDistance() {
        return usonic->GetRange();
    }

    units::inch_t getInchesDistance() {
        return units::inch_t{ usonic->GetRange() };
    }

    void tick() {
        char buf[256] = {};
        snprintf(buf, sizeof(buf), "%d inches", getInchesDistance().convert<double>());

        frc::DataLogManager::Log(buf);
    }
};
