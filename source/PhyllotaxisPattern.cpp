#include "PhyllotaxisPattern.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace phyllotaxis
{

namespace
{
constexpr double twoPi = 2.0 * std::numbers::pi;

constexpr double rotRate = -0.16;
constexpr double paletteRate = 0.008;
constexpr double divisionSweepRate = 0.02536;
constexpr double sweepMin = 1.0;
constexpr double sweepMax = 233.0;
constexpr double sweepShape = 0.99;
constexpr double sweepMid = (sweepMin + sweepMax) / 2.0;
constexpr double sweepAmp = (sweepMax - sweepMin) / 2.0;
constexpr double smoothSeconds = 0.3;
constexpr int stopCount = 7;
constexpr double achromaticChroma = 0.002;
constexpr Rgb base { 21.0 / 255.0, 27.0 / 255.0, 38.0 / 255.0 }; // #151b26, the shader's base colour

const double sweepNorm = std::asin (sweepShape);

double wrapTurnsSigned (double dh) { return dh - std::floor (dh + 0.5); }

double sweepDivisions (double x)
{
    return sweepMid - sweepAmp * std::asin (sweepShape * std::sin (x)) / sweepNorm;
}

double sweepPhaseAt (double v, bool rising)
{
    const double u = std::clamp ((sweepMid - v) / sweepAmp, -1.0, 1.0);
    const double x = std::asin (std::sin (u * sweepNorm) / sweepShape);
    return rising ? std::numbers::pi - x : x;
}

struct Oklch
{
    double L = 0.0;
    double C = 0.0;
    double h = 0.0; // turns
};

Oklch mixOklch (const Oklch& a, const Oklch& b, double t)
{
    double ah = a.h;
    double bh = b.h;
    if (a.C < achromaticChroma)
        ah = bh;
    else if (b.C < achromaticChroma)
        bh = ah;
    const double dh = wrapTurnsSigned (bh - ah);
    return { a.L + (b.L - a.L) * t, a.C + (b.C - a.C) * t, fract (ah + dh * t) };
}

// OKLCH -> sRGB 0..1, quantised to 8 bits like the shader's palette texture and
// gamut-mapped toward the grey of the same lightness
Rgb oklchToSrgb (const Oklch& c)
{
    const double L = clamp01 (c.L);
    const double a = c.C * std::cos (twoPi * c.h);
    const double b = c.C * std::sin (twoPi * c.h);
    const double l_ = L + 0.3963377774 * a + 0.2158037573 * b;
    const double m_ = L - 0.1055613458 * a - 0.0638541728 * b;
    const double s_ = L - 0.0894841775 * a - 1.2914855480 * b;
    const double l = l_ * l_ * l_;
    const double m = m_ * m_ * m_;
    const double s = s_ * s_ * s_;
    const std::array<double, 3> rgb {
        +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    };

    const double grey = L * L * L;
    double scale = 1.0;
    for (const double v : rgb)
    {
        if (v > 1.0)
            scale = std::min (scale, (1.0 - grey) / (v - grey));
        else if (v < 0.0)
            scale = std::min (scale, grey / (grey - v));
    }

    const auto encode = [&] (double v)
    {
        v = clamp01 (grey + scale * (v - grey));
        v = v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow (v, 1.0 / 2.4) - 0.055;
        return std::round (v * 255.0) / 255.0;
    };

    return { encode (rgb[0]), encode (rgb[1]), encode (rgb[2]) };
}

double smootherstep (double x)
{
    x = clamp01 (x);
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

double smoothstep (double e0, double e1, double x)
{
    const double t = clamp01 ((x - e0) / (e1 - e0));
    return t * t * (3.0 - 2.0 * t);
}

double divisionMask (double p)
{
    constexpr double width = 0.498;
    constexpr double peak = width * 0.5;
    const double tri = clamp01 (p < peak ? p / peak : 1.0 - (p - peak) / (width - peak));
    return smootherstep (tri);
}
} // namespace

//==============================================================================
double fract (double x) { return x - std::floor (x); }
double clamp01 (double x) { return x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x); }
double brightness (const Rgb& c) { return std::max ({ c.r, c.g, c.b }); }

//==============================================================================
void Palette::update (const PaletteSettings& settings)
{
    if (valid && settings == current)
        return;

    current = settings;
    valid = true;

    std::array<Oklch, stopCount> stops;
    std::array<double, stopCount> stopPos;
    for (int i = 0; i < stopCount; ++i)
    {
        const double t = i / static_cast<double> (stopCount - 1);
        stopPos[static_cast<size_t> (i)] = t;
        stops[static_cast<size_t> (i)] = { clamp01 (settings.lightness + settings.lightSwing * 0.5 * std::sin (twoPi * t)),
                                           settings.chroma,
                                           fract ((settings.hue + settings.hueSpan * t) / 360.0) };
    }

    // FastLED-style gradient sampling of the stops
    for (size_t i = 0; i < entries.size(); ++i)
    {
        const double pos = static_cast<double> (i) / 15.0;
        size_t k = 0;
        while (k < stops.size() && stopPos[k] < pos)
            ++k;

        Oklch c;
        if (k == 0)
            c = stops.front();
        else if (k == stops.size())
            c = stops.back();
        else
            c = mixOklch (stops[k - 1], stops[k], (pos - stopPos[k - 1]) / (stopPos[k] - stopPos[k - 1]));

        entries[i] = oklchToSrgb (c);
    }
}

Rgb Palette::sample (double pos) const
{
    const double x = fract (pos) * 16.0;
    const auto i = static_cast<size_t> (std::floor (x)) % 16;
    const auto j = (i + 1) % 16;
    const double f = x - std::floor (x);
    const Rgb& a = entries[i];
    const Rgb& b = entries[j];
    return { a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f };
}

//==============================================================================
void Sites::update (int newCount, double newDiscriminant)
{
    newCount = std::clamp (newCount, 2, maxPoints);

    if (newCount != count)
    {
        for (int n = 0; n < newCount; ++n)
            rad[static_cast<size_t> (n)] = std::sqrt (n / static_cast<double> (newCount - 1));
    }
    else if (newDiscriminant == discriminant)
    {
        return;
    }

    const double g = 1.0 - 2.0 / (1.0 + std::sqrt (std::max (newDiscriminant, 0.0)));
    for (int n = 0; n < newCount; ++n)
        ang[static_cast<size_t> (n)] = fract (n * g);

    count = newCount;
    discriminant = newDiscriminant;
}

//==============================================================================
Rgb siteColor (int n, const Sites& sites, const Palette& palette, const ColorFrame& frame)
{
    const double totalPhase = (frame.maskFromIndex ? n / 200.0 : sites.angle (n)) * frame.divisions;
    const double phase = fract (totalPhase + frame.rotPhase);
    const double glow = divisionMask (phase) * (1.0 - smoothstep (0.7, 1.15, sites.radius (n)));
    const Rgb lit = palette.sample (totalPhase + frame.palPhase);

    const auto mix = [glow] (double b, double l) { return clamp01 (b + (l - b) * glow * 0.42 + l * glow * 0.2); };
    return { mix (base.r, lit.r), mix (base.g, lit.g), mix (base.b, lit.b) };
}

//==============================================================================
PatternMotion::PatternMotion()
    : PatternMotion (Targets {})
{
}

PatternMotion::PatternMotion (const Targets& initial)
{
    speedS.value = initial.speed;
    divisionsS.value = initial.divisions;
    discriminantS.value = initial.discriminant;
}

// critically damped smoothing (Game Programming Gems 4), as in palette_blend.h
void PatternMotion::smoothTo (Smoother& s, double target, double omega, double dt, double decay, double epsilon)
{
    const double change = s.value - target;
    const double temp = (s.velocity + omega * change) * dt;
    s.velocity = (s.velocity - omega * temp) * decay;
    s.value = target + (change + temp) * decay;
    if (std::abs (s.value - target) < epsilon)
    {
        s.value = target;
        s.velocity = 0.0;
    }
}

void PatternMotion::step (double dt, const Targets& targets)
{
    const double omega = 2.0 / smoothSeconds;
    const double x = omega * dt;
    const double decay = 1.0 / (1.0 + x + 0.48 * x * x + 0.235 * x * x * x);

    smoothTo (speedS, targets.speed, omega, dt, decay, 1e-4);
    const double t = speedS.value * dt;
    rotPhase = fract (rotPhase + rotRate * t);
    palPhase = fract (palPhase + paletteRate * t);

    if (sweepLocked && ! targets.divLock)
        sweepPhase = sweepPhaseAt (divisionsS.value, std::cos (sweepPhase) < 0.0);

    sweepLocked = targets.divLock;
    if (! sweepLocked)
        sweepPhase = std::fmod (sweepPhase + divisionSweepRate * t, twoPi);

    smoothTo (divisionsS, sweepLocked ? targets.divisions : sweepDivisions (sweepPhase), omega, dt, decay, 1e-4);
    smoothTo (discriminantS, targets.discriminant, omega, dt, decay, 1e-5);
}

void PatternMotion::resetPhases()
{
    rotPhase = 0.0;
    palPhase = 0.0;
    sweepPhase = 0.0;
}

} // namespace phyllotaxis
