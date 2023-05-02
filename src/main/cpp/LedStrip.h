#ifndef ROBOT_LEDSTRIP_H
#define ROBOT_LEDSTRIP_H

#include <random>

#include <frc/motorcontrol/Talon.h>

#include "PerlinNoise.h"

struct Color {
        double r, g, b;

        bool operator==(Color& lhs) const { return r == lhs.r && g == lhs.g && b == lhs.b; }
        bool operator!=(Color& lhs) const { return r != lhs.r || g != lhs.g || b != lhs.b; }

        static Color all(double val) {
            return { val, val, val };
        }
    };

using ColorUpdater = std::function<Color(int64_t now, int64_t deltaTime, Color prevColor)>;

class LedStrip {
public:
    frc::Talon red, green, blue;
    Color lastColor = { 1, 1, 1 };
    ColorUpdater supply;
    double intensityMultiplier = 1.0;


    LedStrip(int redPort, int greenPort, int bluePort, Color color) : 
        red(redPort), green(greenPort), blue(bluePort), supply([=](int64_t, int64_t, Color){ return color; }) {}

    LedStrip(int redPort, int greenPort, int bluePort, ColorUpdater updater) : 
        red(redPort), green(greenPort), blue(bluePort), supply(updater) { }


    void setColor(Color color) {
        if (color != lastColor) {
            red.Set(color.r * intensityMultiplier);
            green.Set(color.g * intensityMultiplier);
            blue.Set(color.b * intensityMultiplier);
            
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
    // Default color
    [](int64_t, int64_t, Color) { return defaultColor; },

    // Breathing blue (Cycles 0.5 to 1.0 on only blue)
    [](int64_t now, int64_t, Color) -> Color { return { 0, 0, (std::abs(std::sin((M_PI * double(now)) / cycleTime)) + 1.0) / 2.0 }; },

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
        return { pn.noise(0, 0, double(now) / 10.0), pn.noise(1.0, 0, double(now) / 10.0), pn.noise(2.0, 0, double(now) / 10.0) };
    }

    // Muddy
};
int currentLedPattern = 0;

#endif