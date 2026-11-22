#include "PhyllotaxisParameters.h"

#include <cmath>

namespace phyllotaxis
{

int ParamSpec::decimals() const noexcept
{
    if (log || step >= 1.0)
        return 0;
    return std::clamp (static_cast<int> (std::ceil (-std::log10 (step) - 1e-9)), 0, 3);
}

double ParamSpec::quantize (double v) const noexcept
{
    if (! std::isfinite (v))
        return def;
    if (isToggle())
        return v >= 0.5 ? 1.0 : 0.0;

    v = std::clamp (v, min, max);
    if (step >= 1.0)
        return std::round (v);

    const double scale = std::pow (10.0, decimals());
    return std::round (std::round (v / step) * step * scale) / scale;
}

double ParamSpec::toNorm (double v) const noexcept
{
    if (log)
        return std::log (v / min) / std::log (max / min);
    return (v - min) / (max - min);
}

double ParamSpec::fromNorm (double u) const noexcept
{
    u = std::clamp (u, 0.0, 1.0);
    if (log)
        return min * std::pow (max / min, u);
    return min + u * (max - min);
}

yup::String ParamSpec::format (double v, bool withUnit) const
{
    if (isToggle())
        return v >= 0.5 ? "on" : "off";

    if (note)
    {
        const auto n = static_cast<int> (std::lround (v));
        return yup::String (n) + " " + noteName (n);
    }

    auto text = yup::String (v, decimals());
    if (decimals() == 0)
        text = yup::String (static_cast<int> (std::lround (v)));

    if (withUnit && ! unit.empty())
        text << " " << yup::String (unit.data(), unit.size());

    return text;
}

yup::String noteName (int note)
{
    static constexpr std::array<const char*, 12> names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int octave = static_cast<int> (std::floor (note / 12.0)) - 1;
    return yup::String (names[static_cast<size_t> (((note % 12) + 12) % 12)]) + yup::String (octave);
}

namespace
{
// "C2", "c#-1", "D#3" or a plain number
float parseValue (const ParamSpec& p, const yup::String& rawText)
{
    const auto text = rawText.trim();
    if (p.isToggle())
    {
        if (text.equalsIgnoreCase ("on") || text.equalsIgnoreCase ("true"))
            return 1.0f;
        if (text.equalsIgnoreCase ("off") || text.equalsIgnoreCase ("false"))
            return 0.0f;
    }

    if (p.note)
    {
        static constexpr std::array<int, 7> semitones { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
        const auto letter = yup::CharacterFunctions::toUpperCase (text[0]);
        if (letter >= 'A' && letter <= 'G')
        {
            int n = semitones[static_cast<size_t> (letter - 'A')];
            auto rest = text.substring (1);
            if (rest.startsWithChar ('#'))
            {
                ++n;
                rest = rest.substring (1);
            }
            else if (rest.startsWithChar ('b'))
            {
                --n;
                rest = rest.substring (1);
            }
            return static_cast<float> (n + 12 * (rest.getIntValue() + 1));
        }
    }

    return static_cast<float> (text.getDoubleValue());
}
} // namespace

yup::AudioParameter::Ptr createParameter (int index)
{
    const ParamSpec& p = spec (index);

    yup::NormalisableRange<float> range (static_cast<float> (p.min), static_cast<float> (p.max), static_cast<float> (p.step));

    // fractional steps snap in double precision: 80 * 0.005f is not 0.4f
    if (p.log || p.step < 1.0)
    {
        range = yup::NormalisableRange<float> (
            static_cast<float> (p.min),
            static_cast<float> (p.max),
            [&p] (float, float, float u) { return static_cast<float> (p.fromNorm (static_cast<double> (u))); },
            [&p] (float, float, float v) { return static_cast<float> (p.toNorm (static_cast<double> (v))); },
            [&p] (float, float, float v) { return static_cast<float> (p.quantize (static_cast<double> (v))); });
    }

    auto builder = yup::AudioParameterBuilder()
                       .withID (yup::String (p.id.data(), p.id.size()))
                       .withName (yup::String::fromUTF8 (p.label.data(), static_cast<int> (p.label.size())))
                       .withHostID (static_cast<yup::uint32> (index + 1))
                       .withRange (range)
                       .withDefault (static_cast<float> (p.def))
                       .withValueToString ([&p] (float v) { return p.format (static_cast<double> (v), true); })
                       .withStringToValue ([&p] (const yup::String& text) { return parseValue (p, text); });

    if (p.isToggle() || p.step >= 1.0)
        builder.withStepped (true);

    if (p.unit == "ms")
        builder.withUnit (yup::AudioParameter::ParameterUnit::Milliseconds);

    return builder.build();
}

} // namespace phyllotaxis
