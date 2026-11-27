#pragma once

#include "PhyllotaxisProcessor.h"

#include <yup_gui/yup_gui.h>

#include <functional>

namespace phyllotaxis
{

/** The spiral, the trigger flashes, the sweep line and the status readout.

    It mirrors the audio thread's pattern from the phases the processor
    publishes, so the dots show the colours the notes were played with. */
class PatternView final : public yup::Component
{
public:
    explicit PatternView (PhyllotaxisProcessor& processor);

    /** Called when the panel button in the corner is clicked. */
    std::function<void()> onPanelButton;
    void setPanelShown (bool shown);

    void clearFlashes();

    void paint (yup::Graphics& g) override;
    void refreshDisplay (double lastFrameTimeSeconds);
    void mouseMove (const yup::MouseEvent& event) override;
    void mouseExit (const yup::MouseEvent& event) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    yup::Rectangle<float> panelButtonBounds() const;
    void updateGeometry();
    void paintStatus (yup::Graphics& g, double nowMs);
    void paintPanelButton (yup::Graphics& g);

    static constexpr double flashMs = 220.0;

    PhyllotaxisProcessor& processor;
    PhyllotaxisProcessor::View view;

    Sites sites;
    Palette palette;

    // unit vectors of the sites, rebuilt only when the layout changes
    std::array<float, maxPoints> unitX {};
    std::array<float, maxPoints> unitY {};
    int geometryCount = 0;
    double geometryDiscriminant = -1.0;

    std::array<double, maxPoints> flashTime {}; // UI clock, ms; 0 = never
    double viewChangedMs = 0.0;
    yup::int64 lastClock = -1;
    double lastNoteMs = 0.0;
    yup::int64 lastNoteClock = -1;

    bool panelShown = true;
    bool hoverButton = false;
    yup::Font statusFont;

    // Calls refreshDisplay() on every frame of the window this is in. Last, so
    // it stops before any of the members above are destroyed.
    yup::FrameAttachment frameAttachment { *this, yup::bindFront (&PatternView::refreshDisplay, this) };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatternView)
};

} // namespace phyllotaxis
