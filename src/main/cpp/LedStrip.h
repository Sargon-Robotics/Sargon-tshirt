#ifndef ROBOT_LEDSTRIP_H
#define ROBOT_LEDSTRIP_H

#include <random>

#include <frc/motorcontrol/Talon.h>
#include <frc/Relay.h>

#include "PerlinNoise.h"

struct Color {
    double r, g, b;

    bool operator==(Color& lhs) const { return r == lhs.r && g == lhs.g && b == lhs.b; }
    bool operator!=(Color& lhs) const { return r != lhs.r || g != lhs.g || b != lhs.b; }

    static Color all(double val) {
        return { val, val, val };
    }
};

Color hsvToRgb(Color hsv) {
    auto [h, s, v] = hsv;
    double r, g, b;

    double c = v * s; // Chroma
    double hPrime = fmod(h / 60.0, 6);
    double x = c * (1 - fabs(fmod(hPrime, 2) - 1));
    double m = v - c;

    if (0 <= hPrime && hPrime < 1) {
        r = c;
        g = x;
        b = 0;
    }
    else if (1 <= hPrime && hPrime < 2) {
        r = x;
        g = c;
        b = 0;
    }
    else if (2 <= hPrime && hPrime < 3) {
        r = 0;
        g = c;
        b = x;
    }
    else if (3 <= hPrime && hPrime < 4) {
        r = 0;
        g = x;
        b = c;
    }
    else if (4 <= hPrime && hPrime < 5) {
        r = x;
        g = 0;
        b = c;
    }
    else if (5 <= hPrime && hPrime < 6) {
        r = c;
        g = 0;
        b = x;
    }
    else { 
        r = 0;
        g = 0;
        b = 0;
    }

    return { r + m, g + m, b + m };
}

Color HSV2RGB(Color _HSV)
{
    return {_HSV.r, _HSV.r, _HSV.r};
}

using ColorUpdater = std::function<Color(int64_t now, int64_t deltaTime, Color prevColor)>;

class LedStrip {
public:
    frc::Talon red, green, blue;
    frc::Relay power;
    Color lastColor = { 1, 1, 1 };
    ColorUpdater supply;
    double intensityMultiplier = 1.0;


    LedStrip(int redPort, int greenPort, int bluePort, int powerPort, Color color) : 
        red(redPort), green(greenPort), blue(bluePort), power(powerPort), supply([=](int64_t, int64_t, Color){ return color; }) {}

    LedStrip(int redPort, int greenPort, int bluePort, int powerPort, ColorUpdater updater) : 
        red(redPort), green(greenPort), blue(bluePort), power(powerPort), supply(updater) { }


    void setColor(Color color) {
        if (color != lastColor || true) {
            red.Set(1.0 - color.r * intensityMultiplier);
            green.Set(1.0 - color.g * intensityMultiplier);
            blue.Set(1.0 - color.b * intensityMultiplier);
            
            lastColor = color;
        }
    }

    void tick(int64_t now, int64_t delta) {
        Color next = supply(now, delta, lastColor);

        setColor(next);
    }

    void setUpdater(ColorUpdater updater) {
        supply = updater;
    }
};

double cycleTime = 1000; // How often to go through a full color cycle
Color defaultColor = { 0, 0, 0.5 };

ColorUpdater defaultLedPatterns[] = {
    // Default tester
    //[](int64_t now, int64_t, Color) { return Color::all((std::sin((M_PI * double(now)) / cycleTime))); },


    

    // Breathing blue (Cycles 0.5 to 1.0 on only blue)
    [](int64_t now, int64_t, Color) -> Color { return { 0, 0, (std::abs(std::sin((M_PI * double(now)) / cycleTime)) + 1.0) / 2.0 }; },

    // Default color
    [](int64_t, int64_t, Color) { return defaultColor; },

    // Rainbow
    [](int64_t now, int64_t, Color) -> Color { 
        static double hue = 0.0;
        hue = std::fmod(hue + 0.1, 1.0);
        return hsvToRgb({hue, 1.0, 1.0});
    },

    // Solid color
    [](int64_t, int64_t, Color) -> Color { return {1, 0, 0}; },
    [](int64_t, int64_t, Color) -> Color { return {0, 1, 0}; },
    [](int64_t, int64_t, Color) -> Color { return {0, 0, 1}; },

    // Absolute sine
    [](int64_t now, int64_t, Color) { return Color::all(std::abs(std::sin((M_PI * double(now)) / cycleTime))); },

    // Random static
    [](int64_t now, int64_t, Color) -> Color { 
        static std::default_random_engine rng;
        std::uniform_real_distribution<double> dist(0.0, 1.0); 
        return { dist(rng), dist(rng), dist(rng) }; 
    },

    // Coherent noise
    [](int64_t now, int64_t, Color) -> Color { 
        const static PerlinNoise pn;
        return { pn.noise(0, 0, double(now) / 10.0), pn.noise(10.0, 0, double(now) / 10.0), pn.noise(20.0, 0, double(now) / 10.0) };
    }

    // Muddy

};
int currentLedPattern = 0;

#endif