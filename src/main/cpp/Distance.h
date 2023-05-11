#pragma once

#include <chrono>
#include <frc/SerialPort.h>
#include <frc/filter/MedianFilter.h>

namespace Distance {
    inline std::optional<frc::SerialPort> serial;
    inline int reading = -1;
    inline int64_t lastSample = -1;
    inline int64_t lastUpdate = {};
    inline frc::MedianFilter<int> filter(10);
    inline int filterSample = 0;
    inline int64_t lastSeenTime = 0;
    inline int prevSamples[2] = {0,0};

    constexpr int SAFE_DISTANCE_MIN = 230;

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
                        filterSample = filter.Calculate(reading);
                        prevSamples[0] = prevSamples[1];
                        prevSamples[1] = reading;

                        if (filterSample < SAFE_DISTANCE_MIN) {
                            lastSeenTime = lastUpdate;
                        }
                        valuesPtr = 0;
                    }
                    else {
                        valuesPtr++;
                    }
                }

                ptr++;
            }

            lastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
    }

    bool safeToShoot() {
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - lastUpdate > 1000) {
            // If sensor has not been read in the last second (unresponsive)
            return false;
        }

        if (now - lastSeenTime < 5000) {
            // If an object has been detected in the last 5 seconds
            return false;
        }

        if (prevSamples[0] < SAFE_DISTANCE_MIN && prevSamples[1] < SAFE_DISTANCE_MIN) {
            // Two positive readings in a row
            return false;
        }

        return filterSample > SAFE_DISTANCE_MIN;
    }
}
