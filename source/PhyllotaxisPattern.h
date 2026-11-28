#pragma once

#include <array>
#include <cstddef>

// The pattern of phyllotaxis_moire_sequencer.js, ported line for line in double
// precision: the OKLCH palette, the site layout, the animated phases and the
// colour each site is given. Nothing here allocates, so the audio thread runs
// it as is and the editor runs its own copy to draw.
namespace phyllotaxis
{

inline constexpr int maxPoints = 9001;

struct Rgb
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

double fract (double x);
double clamp01 (double x);

/** max (r, g, b): the HSB value the sequencer turns into velocity. */
double brightness (const Rgb& c);

//==============================================================================
struct PaletteSettings
{
    double hue = 200.0;
    double hueSpan = 60.0;
    double lightness = 0.66;
    double lightSwing = 0.4;
    double chroma = 0.11;

    bool operator== (const PaletteSettings&) const = default;
};

/** The 16 entry synth palette, sampled like the shader's LINEAR + REPEAT lookup. */
class Palette
{
public:
    /** Rebuilds the entries when the settings changed. */
    void update (const PaletteSettings& settings);

    Rgb sample (double pos) const;

private:
    std::array<Rgb, 16> entries {};
    PaletteSettings current;
    bool valid = false;
};

//==============================================================================
/** Site n sits at n golden angles, at radius sqrt (n / (count - 1)). */
class Sites
{
public:
    /** Recomputes what changed. Radii only depend on the count, angles on both. */
    void update (int count, double discriminant);

    int size() const noexcept { return count; }
    double radius (int n) const noexcept { return rad[static_cast<std::size_t> (n)]; }

    /** In turns, counter-clockwise from +x. */
    double angle (int n) const noexcept { return ang[static_cast<std::size_t> (n)]; }

private:
    std::array<double, maxPoints> rad {};
    std::array<double, maxPoints> ang {};
    int count = 0;
    double discriminant = -1.0;
};

//==============================================================================
/** What a site's colour depends on besides the site itself. */
struct ColorFrame
{
    double divisions = 56.0;
    double rotPhase = 0.0;
    double palPhase = 0.0;
    bool maskFromIndex = false;
};

/** The colour the shader gives site n's cell, at the site, as sRGB 0..1. */
Rgb siteColor (int n, const Sites& sites, const Palette& palette, const ColorFrame& frame);

//==============================================================================
/** The animated part of the pattern: phases plus critically damped smoothing. */
class PatternMotion
{
public:
    struct Targets
    {
        double speed = 0.7;
        double divisions = 56.0;
        bool divLock = true;
        double discriminant = 5.0;
    };

    PatternMotion();
    explicit PatternMotion (const Targets& initial);

    /** stepPattern() of the original, without the site and palette updates. */
    void step (double dt, const Targets& targets);

    /** Phases back to zero, the smoothed values kept. */
    void resetPhases();

    double getRotPhase() const noexcept { return rotPhase; }
    double getPalPhase() const noexcept { return palPhase; }
    double getDivisions() const noexcept { return divisionsS.value; }
    double getDiscriminant() const noexcept { return discriminantS.value; }

private:
    struct Smoother
    {
        double value = 0.0;
        double velocity = 0.0;
    };

    static void smoothTo (Smoother& s, double target, double omega, double dt, double decay, double epsilon);

    double rotPhase = 0.0;
    double palPhase = 0.0;
    double sweepPhase = 0.0;
    bool sweepLocked = true;
    Smoother speedS;
    Smoother divisionsS;
    Smoother discriminantS;
};

} // namespace phyllotaxis
