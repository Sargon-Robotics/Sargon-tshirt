#pragma once

#include <frc/SerialPort.h>

namespace Distance {
    inline std::optional<frc::SerialPort> serial;
    inline int reading = -1;
    inline int64_t lastSample = -1;

    constexpr int SAFE_DISTANCE_MIN = 260;

    void init() {
        serial = frc::SerialPort(9600);
        serial->SetTimeout(units::time::second_t(0));

    }

    void tick() {
        size_t bytesRead = 0;
        char buf[255];
        while ((bytesRead = serial->Read(buf, sizeof(buf)))) {
            char* ptr = buf;
            char values[4] = {};
            size_t valuesPtr = 0;

            while (ptr < buf + bytesRead) {
                if (*ptr == 'R') {
                    valuesPtr = 0;
                }
                else if (*ptr >= '0' && *ptr <= '9') {
                    values[valuesPtr] = *ptr - '0';

                    if (valuesPtr == 2) {
                        reading = values[0] * 100 + values[1] * 10 + values[2];
                        valuesPtr = 0;
                    }
                    else {
                        valuesPtr++;
                    }
                }

                ptr++;
            }
        }
    }

    bool safeToShoot() {

    }
}