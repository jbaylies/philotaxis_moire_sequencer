#pragma once

#include <yup_audio_processors/yup_audio_processors.h>

#include <array>
#include <string_view>

namespace phyllotaxis
{

/** Parameter indices, in panel order. Never reorder: the host IDs are index + 1. */
enum ParamIndex : int
{
    pPlay,
    pRpm,
    pNoteMin,
    pNoteMax,
    pDuration,
    pChannel,
    pGate,
    pSpeed,
    pDivisions,
    pDivLock,
    pMaskType,
    pPoints,
    pDiscriminant,
    pHue,
    pHueSpan,
    pLightness,
    pLightSwing,
    pChroma,
    pFollowTransport,
    numParams
};

struct ParamSpec
{
    enum class Kind
    {
        slider,
        toggle
    };

    std::string_view id;
    std::string_view label;
    Kind kind = Kind::slider;
    double min = 0.0;
    double max = 1.0;
    double step = 1.0;
    double def = 0.0;
    bool log = false;
    bool note = false;
    std::string_view unit = {};

    bool isToggle() const noexcept { return kind == Kind::toggle; }
    bool isBipolar() const noexcept { return min < 0.0 && max > 0.0; }

    /** Decimal places shown, as decimalsFor() in the original. */
    int decimals() const noexcept;

    /** Snaps to the step in double precision, so 0.66 is 0.66 and not 0.6600000262. */
    double quantize (double v) const noexcept;

    /** Slider position 0..1 and back; log sliders as on the web panel. */
    double toNorm (double v) const noexcept;
    double fromNorm (double u) const noexcept;

    /** "36 C2" for notes, "150 ms", "on" / "off" for toggles. */
    yup::String format (double v, bool withUnit) const;
};

using Kind = ParamSpec::Kind;

// The pattern, spiral and colour ranges and defaults match the links page's shader panel
inline constexpr std::array<ParamSpec, numParams> paramSpecs { {
    { "play", "play", Kind::toggle, 0, 1, 1, 1 },
    { "rpm", "line rpm", Kind::slider, -120, 120, 0.1, 6 },
    { "noteMin", "note min", Kind::slider, 0, 127, 1, 36, false, true },
    { "noteMax", "note max", Kind::slider, 0, 127, 1, 84, false, true },
    { "duration", "note length", Kind::slider, 1, 4000, 1, 150, true, false, "ms" },
    { "channel", "midi channel", Kind::slider, 1, 16, 1, 1 },
    { "gate", "velocity gate", Kind::slider, 0, 127, 1, 0 },
    { "speed", "speed", Kind::slider, -10, 10, 0.1, 0.7 },
    { "divisions", "divisions", Kind::slider, 1, 233, 0.5, 56 },
    { "divLock", "lock divisions (off = sweep)", Kind::toggle, 0, 1, 1, 1 },
    { "maskType", "index-from-centre mask", Kind::toggle, 0, 1, 1, 0 },
    { "points", "points", Kind::slider, 2, 9001, 1, 201, true },
    { "discriminant", "\xcf\x86 discriminant", Kind::slider, 1, 9, 0.001, 5 },
    { "hue", "hue", Kind::slider, 0, 360, 1, 200 },
    { "hueSpan", "hue span", Kind::slider, 0, 360, 1, 60 },
    { "lightness", "lightness", Kind::slider, 0, 1, 0.01, 0.66 },
    { "lightSwing", "light swing", Kind::slider, 0, 1, 0.01, 0.4 },
    { "chroma", "chroma", Kind::slider, 0, 0.4, 0.005, 0.11 },
    { "followTransport", "follow host transport", Kind::toggle, 0, 1, 1, 1 },
} };

inline const ParamSpec& spec (int index) { return paramSpecs[static_cast<size_t> (index)]; }

/** "C2" for 36. */
yup::String noteName (int note);

/** Builds the host parameter for a spec, with its range, steps and text conversions. */
yup::AudioParameter::Ptr createParameter (int index);

} // namespace phyllotaxis
