#pragma once

#include "PhyllotaxisPattern.h"

#include <yup_gui/yup_gui.h>

// Flat, matte and quiet, so the spiral is the only colour on screen.
namespace phyllotaxis::theme
{

inline constexpr yup::Color canvas { 0xff0b0e14 };
inline constexpr yup::Color panel { 0xff13171f };
inline constexpr yup::Color hairline { 0xff222834 };
inline constexpr yup::Color text { 0xffdce0eb };
inline constexpr yup::Color dim { 0xff8c96a8 };
inline constexpr yup::Color faint { 0xff5a6374 };
inline constexpr yup::Color track { 0xff29303f };
inline constexpr yup::Color accent { 0xff78b5dc };

inline yup::Color toColor (const Rgb& c, float alpha = 1.0f)
{
    const auto byte = [] (double v) { return static_cast<yup::uint8> (std::lround (clamp01 (v) * 255.0)); };
    return yup::Color (byte (c.r), byte (c.g), byte (c.b)).withAlpha (alpha);
}

/** Labels: Roboto Flex, slightly narrowed. Building a variable instance is
    costly, so components keep the fonts they use. */
inline yup::Font labelFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withAxisValues ({ { "wdth", 88.0f }, { "wght", 430.0f } }).withHeight (height);
}

/** Numbers: JetBrains Mono, so changing values never shift sideways. */
inline yup::Font monoFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont().withHeight (height);
}

inline float textWidth (const yup::String& s, const yup::Font& font)
{
    yup::StyledText styled;
    {
        auto modifier = styled.startUpdate();
        modifier.setWrap (yup::StyledText::noWrap);
        modifier.appendText (s, font);
    }
    return styled.getComputedTextBounds().getWidth();
}

} // namespace phyllotaxis::theme
