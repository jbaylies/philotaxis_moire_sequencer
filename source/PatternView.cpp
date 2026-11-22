#include "PatternView.h"
#include "PhyllotaxisTheme.h"

#include <cmath>
#include <numbers>

namespace phyllotaxis
{

namespace
{
double nowMs() { return yup::Time::getMillisecondCounterHiRes(); }

// how far back an audio clock time lies, in ms
double msAgo (yup::int64 time, const PhyllotaxisProcessor::View& view)
{
    return std::max (0.0, static_cast<double> (view.clock - time) * 1000.0 / view.sampleRate);
}
} // namespace

PatternView::PatternView (PhyllotaxisProcessor& p)
    : processor (p)
    , statusFont (theme::monoFont (10.5f))
{
    setOpaque (true);
    view = processor.getView();
}

void PatternView::setPanelShown (bool shown)
{
    panelShown = shown;
    repaint();
}

void PatternView::clearFlashes()
{
    flashTime.fill (0.0);
}

//==============================================================================
void PatternView::refreshDisplay (double)
{
    const double now = nowMs();

    view = processor.getView();
    if (view.clock != lastClock)
    {
        lastClock = view.clock;
        viewChangedMs = now;
    }

    FlashQueue::Flash flash;
    while (processor.getFlashQueue().pop (flash))
    {
        if (flash.site >= 0 && flash.site < maxPoints)
            flashTime[static_cast<size_t> (flash.site)] = now - msAgo (flash.time, view);
    }

    if (view.lastPitch >= 0 && view.lastNoteTime != lastNoteClock)
    {
        lastNoteClock = view.lastNoteTime;
        lastNoteMs = now - msAgo (view.lastNoteTime, view);
    }

    repaint();
}

void PatternView::updateGeometry()
{
    if (sites.size() == geometryCount && view.discriminant == geometryDiscriminant)
        return;

    if (sites.size() != geometryCount)
        clearFlashes();

    for (int n = 0; n < sites.size(); ++n)
    {
        const double a = 2.0 * std::numbers::pi * sites.angle (n);
        unitX[static_cast<size_t> (n)] = static_cast<float> (sites.radius (n) * std::cos (a));
        unitY[static_cast<size_t> (n)] = static_cast<float> (sites.radius (n) * std::sin (a));
    }

    geometryCount = sites.size();
    geometryDiscriminant = view.discriminant;
}

//==============================================================================
void PatternView::paint (yup::Graphics& g)
{
    const double now = nowMs();

    g.setFillColor (theme::canvas);
    g.fillAll();

    sites.update (static_cast<int> (processor.param (pPoints)), view.discriminant);
    updateGeometry();
    palette.update ({ processor.param (pHue), processor.param (pHueSpan), processor.param (pLightness),
                      processor.param (pLightSwing), processor.param (pChroma) });

    const ColorFrame frame { view.divisions, view.rotPhase, view.palPhase, processor.toggle (pMaskType) };

    const auto bounds = getLocalBounds();
    const float cx = bounds.getCenterX();
    const float cy = bounds.getCenterY();
    const float R = std::max (10.0f, std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 22.0f);
    const int count = sites.size();

    // rim
    g.setStrokeColor (theme::track.withAlpha (0.6f));
    g.setStrokeWidth (1.0f);
    g.strokeEllipse (cx - R, cy - R, 2.0f * R, 2.0f * R);

    // dot size from the spacing of equal-area cells
    const float spacing = R * std::sqrt (std::numbers::pi_v<float> / static_cast<float> (count));
    const float dr = std::clamp (spacing * 0.36f, 0.75f, 10.0f);
    const bool square = dr < 1.6f;

    for (int n = 0; n < count; ++n)
    {
        const float x = cx + R * unitX[static_cast<size_t> (n)];
        const float y = cy - R * unitY[static_cast<size_t> (n)];
        g.setFillColor (theme::toColor (siteColor (n, sites, palette, frame)));

        if (square)
            g.fillRect (x - dr, y - dr, 2.0f * dr, 2.0f * dr);
        else
            g.fillEllipse (x - dr, y - dr, 2.0f * dr, 2.0f * dr);
    }

    // trigger flashes
    g.setStrokeWidth (std::max (1.0f, dr * 0.35f));
    for (int n = 0; n < count; ++n)
    {
        const double t = flashTime[static_cast<size_t> (n)];
        const double age = now - t;
        if (t == 0.0 || age > flashMs)
            continue;

        const auto f = static_cast<float> (1.0 - age / flashMs);
        const float x = cx + R * unitX[static_cast<size_t> (n)];
        const float y = cy - R * unitY[static_cast<size_t> (n)];
        const float rr = dr + 1.0f + dr * 1.2f * (1.0f - f);
        g.setStrokeColor (yup::Colors::white.withAlpha (0.75f * f));
        g.strokeEllipse (x - rr, y - rr, 2.0f * rr, 2.0f * rr);
    }

    // the sweep line, carried forward between audio blocks so it moves evenly
    double linePhase = view.linePhase;
    if (view.running)
        linePhase -= processor.param (pRpm) / 60.0 * std::min (now - viewChangedMs, 50.0) / 1000.0;

    const double la = 2.0 * std::numbers::pi * linePhase;
    g.setStrokeWidth (1.5f);
    g.setStrokeColor (yup::Colors::white.withAlpha (view.running ? 0.85f : 0.35f));
    g.strokeLine (cx, cy, cx + (R + 6.0f) * static_cast<float> (std::cos (la)), cy - (R + 6.0f) * static_cast<float> (std::sin (la)));

    paintStatus (g, now);
    paintPanelButton (g);
}

// "▶ 201 pts · 6.0 rpm · C3 v94": a filled triangle while playing, an open
// one while waiting for the host transport, a square when stopped
void PatternView::paintStatus (yup::Graphics& g, double now)
{
    const float x = 14.0f;
    const float baseline = getHeight() - 14.0f;
    const yup::Rectangle<float> icon (x, baseline - 7.0f, 7.0f, 7.0f);

    const bool playOn = processor.toggle (pPlay);
    g.setFillColor (theme::dim);
    g.setStrokeColor (theme::dim);
    g.setStrokeWidth (1.0f);

    if (! playOn)
    {
        g.fillRect (icon.reduced (0.5f));
    }
    else
    {
        yup::Path triangle;
        triangle.moveTo (icon.getX(), icon.getY());
        triangle.lineTo (icon.getRight(), icon.getCenterY());
        triangle.lineTo (icon.getX(), icon.getBottom());
        triangle.close();

        if (view.running)
            g.fillPath (triangle);
        else
            g.strokePath (triangle);
    }

    const auto separator = yup::String::fromUTF8 (" \xc2\xb7 ");
    auto status = yup::String (sites.size()) + " pts" + separator + yup::String (processor.param (pRpm), 1) + " rpm";
    if (view.lastPitch >= 0 && now - lastNoteMs < 1500.0)
        status << separator << noteName (view.lastPitch) << " v" << view.lastVelocity;

    g.setFillColor (theme::dim);
    g.fillFittedText (status, statusFont, { x + 13.0f, baseline - 11.0f, getWidth() - x - 60.0f, 14.0f }, yup::Justification::centerLeft);
}

yup::Rectangle<float> PatternView::panelButtonBounds() const
{
    return { getWidth() - 30.0f, 10.0f, 20.0f, 20.0f };
}

void PatternView::paintPanelButton (yup::Graphics& g)
{
    const auto b = panelButtonBounds();
    g.setStrokeWidth (1.0f);
    g.setStrokeColor (hoverButton ? theme::dim : theme::hairline);
    g.strokeRect (b.reduced (0.5f));

    // a chevron pointing where the panel will go
    const float dir = panelShown ? 1.0f : -1.0f;
    const float cx = b.getCenterX();
    const float cy = b.getCenterY();
    yup::Path chevron;
    chevron.moveTo (cx - 2.0f * dir, cy - 4.0f);
    chevron.lineTo (cx + 2.0f * dir, cy);
    chevron.lineTo (cx - 2.0f * dir, cy + 4.0f);
    g.setStrokeColor (hoverButton ? theme::text : theme::dim);
    g.strokePath (chevron);
}

//==============================================================================
void PatternView::mouseMove (const yup::MouseEvent& event)
{
    const bool over = panelButtonBounds().contains (event.getPosition());
    if (over != hoverButton)
    {
        hoverButton = over;
        setMouseCursor (over ? yup::MouseCursor::Hand : yup::MouseCursor::Default);
    }
}

void PatternView::mouseExit (const yup::MouseEvent&)
{
    hoverButton = false;
    setMouseCursor (yup::MouseCursor::Default);
}

void PatternView::mouseDown (const yup::MouseEvent& event)
{
    if (panelButtonBounds().contains (event.getPosition()) && onPanelButton)
        onPanelButton();
}

} // namespace phyllotaxis
