#pragma once

#include "PhyllotaxisProcessor.h"

#include <yup_gui/yup_gui.h>

#include <functional>
#include <vector>

namespace phyllotaxis
{

/** The sidebar: every parameter as a flat slider or toggle, grouped as on the
    web panel, with Reset and Defaults on top.

    Click or drag a slider to set it, Shift-drag for fine steps, double-click
    anything to return it to its default. Changes go to the host as gestures. */
class ParameterPanel final : public yup::Component
{
public:
    explicit ParameterPanel (PhyllotaxisProcessor& processor);

    static constexpr float width = 232.0f;

    /** Called after Reset, so the pattern can drop its flashes. */
    std::function<void()> onReset;

    void paint (yup::Graphics& g) override;
    void resized() override;
    void refreshDisplay (double lastFrameTimeSeconds) override;

    void mouseMove (const yup::MouseEvent& event) override;
    void mouseExit (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;
    void mouseDoubleClick (const yup::MouseEvent& event) override;

private:
    enum class RowType
    {
        header,
        section,
        slider,
        toggle,
        swatch
    };

    struct Row
    {
        RowType type;
        int param = -1;
        const char* title = nullptr;
        yup::Rectangle<float> bounds {};
    };

    enum class Button
    {
        none,
        reset,
        defaults
    };

    static float rowHeight (RowType type);
    const Row* rowAt (yup::Point<float> position) const;
    Button buttonAt (yup::Point<float> position) const;
    yup::Rectangle<float> trackBounds (const Row& row) const;

    /** The divisions slider follows the sweep while unlocked. */
    bool isFollowingSweep (int index) const;
    double shownValue (int index) const;

    void setFromUI (int index, double value);
    void setWithGesture (int index, double value);

    void paintHeader (yup::Graphics& g, const Row& row);
    void paintSection (yup::Graphics& g, const Row& row);
    void paintSlider (yup::Graphics& g, const Row& row);
    void paintToggle (yup::Graphics& g, const Row& row);
    void paintSwatch (yup::Graphics& g, const Row& row);
    void paintButton (yup::Graphics& g, yup::Rectangle<float> bounds, const yup::String& text, bool hover);

    PhyllotaxisProcessor& processor;
    std::vector<Row> rows;
    yup::Rectangle<float> resetBounds;
    yup::Rectangle<float> defaultsBounds;

    Palette palette;
    std::array<double, numParams> painted {};
    double paintedDivisions = 0.0;

    int dragParam = -1;
    double dragNorm = 0.0;
    float dragLastX = 0.0f;
    const Row* hoverRow = nullptr;
    Button hoverButton = Button::none;

    yup::Font labelFont;
    yup::Font valueFont;
    yup::Font sectionFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterPanel)
};

} // namespace phyllotaxis
