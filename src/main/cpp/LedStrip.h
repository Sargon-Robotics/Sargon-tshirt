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

Color hsvToRgb(double h, double s, double v) {
    if (s == 0.0) { return {v, v, v};}

    int i = int(h * 6.0);
    double f = (h * 6.0) - i;
    double p = v * (1.0 - s);
    double q = v * (1.0 - s * f);
    double t = v * (1.0 - s * (1.0 - f));
    i %= 6;

    switch (i) {
        case 0: return { v, t, p };
        case 1: return { q, v, p };
        case 2: return { p, v, t };
        case 3: return { p, q, v };
        case 4: return { t, p, v };
        case 5: return { v, p, q };
        default: {
            return { v, p, q };
        }
    }
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

double cycleTime = 2000; // How often to go through a full color cycle
Color defaultColor = { 0, 0, 0.5 };

ColorUpdater defaultLedPatterns[] = {
    // Default tester
    //[](int64_t now, int64_t, Color) { return Color::all((std::sin((M_PI * double(now)) / cycleTime))); },


    

    // Breathing blue (Cycles 0.5 to 1.0 on only blue)
    [](int64_t now, int64_t, Color) -> Color { return { 0, 0, (std::abs(std::sin((M_PI * double(now)) / cycleTime)) + 0.5) / 2.0 }; },

    // Default color
    [](int64_t, int64_t, Color) { return defaultColor; },

    // Rainbow
    [](int64_t now, int64_t, Color) -> Color { 
        static double hue = 0.0;
        hue = std::fmod(hue + 0.02, 1.0);
        return hsvToRgb(hue, 1.0, 1.0);
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
        return { pn.noise(0, 0, double(now) / 1000), pn.noise(10.0, 0, double(now) / 1000.0), pn.noise(20.0, 0, double(now) / 1000.0) };
    }

    // Muddy

};
int currentLedPattern = 0;

#endif