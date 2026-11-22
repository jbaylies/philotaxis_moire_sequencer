#include "ParameterPanel.h"
#include "PhyllotaxisTheme.h"

#include <cmath>

namespace phyllotaxis
{

namespace
{
constexpr float padX = 14.0f;
constexpr float bottomPad = 10.0f;
} // namespace

ParameterPanel::ParameterPanel (PhyllotaxisProcessor& p)
    : processor (p)
    , labelFont (theme::labelFont (11.5f))
    , valueFont (theme::monoFont (10.5f))
    , sectionFont (theme::monoFont (9.0f))
{
    setOpaque (true);

    const auto section = [this] (const char* title) { rows.push_back ({ RowType::section, -1, title }); };
    const auto slider = [this] (int index) { rows.push_back ({ RowType::slider, index }); };
    const auto toggle = [this] (int index) { rows.push_back ({ RowType::toggle, index }); };

    rows.push_back ({ RowType::header });
    section ("sequencer");
    toggle (pPlay);
    toggle (pFollowTransport);
    for (const int i : { pRpm, pNoteMin, pNoteMax, pDuration, pChannel, pGate })
        slider (i);
    section ("pattern");
    slider (pSpeed);
    slider (pDivisions);
    toggle (pDivLock);
    toggle (pMaskType);
    section ("spiral");
    slider (pPoints);
    slider (pDiscriminant);
    section ("color");
    rows.push_back ({ RowType::swatch });
    for (const int i : { pHue, pHueSpan, pLightness, pLightSwing, pChroma })
        slider (i);
}

float ParameterPanel::rowHeight (RowType type)
{
    switch (type)
    {
        case RowType::header: return 42.0f;
        case RowType::section: return 28.0f;
        case RowType::slider: return 30.0f;
        case RowType::toggle: return 21.0f;
        case RowType::swatch: return 16.0f;
    }
    return 0.0f;
}

// rows top to bottom, squeezed to fit the height
void ParameterPanel::resized()
{
    float total = bottomPad;
    for (const auto& row : rows)
        total += rowHeight (row.type);

    const float k = std::min (1.0f, getHeight() / total);
    float y = 0.0f;
    for (auto& row : rows)
    {
        const float h = rowHeight (row.type) * k;
        row.bounds = row.type == RowType::header ? yup::Rectangle<float> (0.0f, y, getWidth(), h)
                                                 : yup::Rectangle<float> (padX, y, getWidth() - 2.0f * padX, h);
        y += h;
    }

    const auto& header = rows.front().bounds;
    const float buttonH = 19.0f;
    const float buttonY = header.getCenterY() - buttonH * 0.5f;
    const float defaultsW = theme::textWidth ("defaults", labelFont) + 16.0f;
    const float resetW = theme::textWidth ("reset", labelFont) + 16.0f;
    defaultsBounds = { getWidth() - padX - defaultsW, buttonY, defaultsW, buttonH };
    resetBounds = { defaultsBounds.getX() - 6.0f - resetW, buttonY, resetW, buttonH };
}

//==============================================================================
bool ParameterPanel::isFollowingSweep (int index) const
{
    return index == pDivisions && ! processor.toggle (pDivLock);
}

double ParameterPanel::shownValue (int index) const
{
    return isFollowingSweep (index) ? processor.getView().divisions : processor.param (index);
}

void ParameterPanel::refreshDisplay (double)
{
    bool changed = false;
    for (int i = 0; i < numParams; ++i)
    {
        const double v = processor.param (i);
        changed = changed || v != painted[static_cast<size_t> (i)];
        painted[static_cast<size_t> (i)] = v;
    }

    const double divisions = shownValue (pDivisions);
    changed = changed || divisions != paintedDivisions;
    paintedDivisions = divisions;

    if (changed)
        repaint();
}

//==============================================================================
void ParameterPanel::paint (yup::Graphics& g)
{
    g.setFillColor (theme::panel);
    g.fillAll();

    // the seam against the canvas
    g.setFillColor (theme::hairline);
    g.fillRect (0.0f, 0.0f, 1.0f, getHeight());

    palette.update ({ processor.param (pHue), processor.param (pHueSpan), processor.param (pLightness),
                      processor.param (pLightSwing), processor.param (pChroma) });

    for (const auto& row : rows)
    {
        switch (row.type)
        {
            case RowType::header: paintHeader (g, row); break;
            case RowType::section: paintSection (g, row); break;
            case RowType::slider: paintSlider (g, row); break;
            case RowType::toggle: paintToggle (g, row); break;
            case RowType::swatch: paintSwatch (g, row); break;
        }
    }
}

void ParameterPanel::paintHeader (yup::Graphics& g, const Row& row)
{
    g.setFillColor (theme::text);
    g.fillFittedText ("PHYLLOTAXIS", sectionFont.withHeight (9.5f), { padX, row.bounds.getY(), 120.0f, row.bounds.getHeight() }, yup::Justification::centerLeft);

    paintButton (g, resetBounds, "reset", hoverButton == Button::reset);
    paintButton (g, defaultsBounds, "defaults", hoverButton == Button::defaults);

    g.setFillColor (theme::hairline);
    g.fillRect (1.0f, row.bounds.getBottom() - 1.0f, getWidth() - 1.0f, 1.0f);
}

void ParameterPanel::paintButton (yup::Graphics& g, yup::Rectangle<float> bounds, const yup::String& text, bool hover)
{
    g.setStrokeWidth (1.0f);
    g.setStrokeColor (hover ? theme::dim : theme::track);
    g.strokeRect (bounds.reduced (0.5f));
    g.setFillColor (hover ? theme::text : theme::dim);
    g.fillFittedText (text, labelFont.withHeight (10.5f), bounds, yup::Justification::center);
}

// "SEQUENCER ─────────" sitting low in its row
void ParameterPanel::paintSection (yup::Graphics& g, const Row& row)
{
    const auto title = yup::String (row.title).toUpperCase();
    const auto textArea = row.bounds.withTrimmedTop (row.bounds.getHeight() * 0.35f);
    g.setFillColor (theme::faint);
    g.fillFittedText (title, sectionFont, textArea, yup::Justification::centerLeft);

    const float ruleX = row.bounds.getX() + theme::textWidth (title, sectionFont) + 8.0f;
    g.setFillColor (theme::hairline);
    g.fillRect (ruleX, std::round (textArea.getCenterY()), row.bounds.getRight() - ruleX, 1.0f);
}

yup::Rectangle<float> ParameterPanel::trackBounds (const Row& row) const
{
    return { row.bounds.getX(), std::round (row.bounds.getY() + row.bounds.getHeight() * 0.72f), row.bounds.getWidth(), 3.0f };
}

void ParameterPanel::paintSlider (yup::Graphics& g, const Row& row)
{
    const ParamSpec& p = spec (row.param);
    const bool following = isFollowingSweep (row.param);
    const bool hover = hoverRow == &row || dragParam == row.param;
    const double value = shownValue (row.param);

    const auto textArea = row.bounds.withHeight (row.bounds.getHeight() * 0.6f);
    g.setFillColor (hover ? theme::text : theme::dim);
    g.fillFittedText (yup::String::fromUTF8 (p.label.data(), static_cast<int> (p.label.size())), labelFont, textArea, yup::Justification::centerLeft);
    g.setFillColor (following ? theme::dim : theme::text);
    g.fillFittedText (p.format (value, true), valueFont, textArea, yup::Justification::centerRight);

    const auto track = trackBounds (row);
    g.setFillColor (hover ? theme::track.brighter (0.15f) : theme::track);
    g.fillRect (track);

    // bipolar sliders fill from zero, with a tick there
    const auto u = static_cast<float> (clamp01 (p.toNorm (value)));
    const auto zero = p.isBipolar() ? static_cast<float> (clamp01 (p.toNorm (0.0))) : 0.0f;
    const float x0 = track.getX() + track.getWidth() * std::min (zero, u);
    const float x1 = track.getX() + track.getWidth() * std::max (zero, u);

    if (p.isBipolar())
    {
        g.setFillColor (theme::faint);
        g.fillRect (std::round (track.getX() + track.getWidth() * zero), track.getY() - 3.0f, 1.0f, track.getHeight() + 6.0f);
    }

    g.setFillColor (theme::accent.withAlpha (following ? 0.45f : 1.0f));
    g.fillRect (x0, track.getY(), x1 - x0, track.getHeight());

    // the value cursor
    g.setFillColor (following ? theme::dim : theme::text);
    g.fillRect (std::round (track.getX() + track.getWidth() * u - 1.0f), track.getY() - 3.0f, 2.0f, track.getHeight() + 6.0f);
}

void ParameterPanel::paintToggle (yup::Graphics& g, const Row& row)
{
    const ParamSpec& p = spec (row.param);
    const bool on = processor.toggle (row.param);
    const bool hover = hoverRow == &row;

    const float size = 9.0f;
    const yup::Rectangle<float> box (row.bounds.getX(), std::round (row.bounds.getCenterY() - size * 0.5f), size, size);

    if (on)
    {
        g.setFillColor (theme::accent);
        g.fillRect (box);
    }
    else
    {
        g.setStrokeWidth (1.0f);
        g.setStrokeColor (hover ? theme::dim : theme::faint);
        g.strokeRect (box.reduced (0.5f));
    }

    g.setFillColor (on || hover ? theme::text : theme::dim);
    g.fillFittedText (yup::String::fromUTF8 (p.label.data(), static_cast<int> (p.label.size())), labelFont,
                      row.bounds.withTrimmedLeft (size + 8.0f), yup::Justification::centerLeft);
}

void ParameterPanel::paintSwatch (yup::Graphics& g, const Row& row)
{
    constexpr int cells = 32;
    const auto area = row.bounds.reduced (0.0f, 3.0f);
    const float w = area.getWidth() / cells;

    for (int i = 0; i < cells; ++i)
    {
        g.setFillColor (theme::toColor (palette.sample (i / static_cast<double> (cells))));
        g.fillRect (area.getX() + i * w, area.getY(), w + 0.5f, area.getHeight());
    }
}

//==============================================================================
const ParameterPanel::Row* ParameterPanel::rowAt (yup::Point<float> position) const
{
    for (const auto& row : rows)
    {
        if (row.param >= 0 && position.getY() >= row.bounds.getY() && position.getY() < row.bounds.getBottom())
            return &row;
    }
    return nullptr;
}

ParameterPanel::Button ParameterPanel::buttonAt (yup::Point<float> position) const
{
    if (resetBounds.contains (position))
        return Button::reset;
    if (defaultsBounds.contains (position))
        return Button::defaults;
    return Button::none;
}

void ParameterPanel::setWithGesture (int index, double value)
{
    const auto& parameter = processor.parameter (index);
    const auto snapped = static_cast<float> (spec (index).quantize (value));

    if (dragParam == index)
    {
        parameter->setValueNotifyingHost (snapped);
        return;
    }

    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (snapped);
    parameter->endChangeGesture();
}

// as setParam (..., fromUI) in the original: moving divisions claims its lock,
// and locking takes the value the sweep is showing right now
void ParameterPanel::setFromUI (int index, double value)
{
    if (index == pDivLock && value >= 0.5 && ! processor.toggle (pDivLock))
        setWithGesture (pDivisions, processor.getView().divisions);

    if (index == pDivisions && ! processor.toggle (pDivLock))
        setWithGesture (pDivLock, 1.0);

    setWithGesture (index, value);
    repaint();
}

//==============================================================================
void ParameterPanel::mouseMove (const yup::MouseEvent& event)
{
    const auto* row = rowAt (event.getPosition());
    const auto button = buttonAt (event.getPosition());
    if (row == hoverRow && button == hoverButton)
        return;

    hoverRow = row;
    hoverButton = button;
    const bool clickable = button != Button::none || (row != nullptr && row->type == RowType::toggle);
    setMouseCursor (clickable ? yup::MouseCursor::Hand : yup::MouseCursor::Default);
    repaint();
}

void ParameterPanel::mouseExit (const yup::MouseEvent&)
{
    hoverRow = nullptr;
    hoverButton = Button::none;
    setMouseCursor (yup::MouseCursor::Default);
    repaint();
}

void ParameterPanel::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();

    if (const auto button = buttonAt (position); button != Button::none)
    {
        if (button == Button::reset)
        {
            processor.requestReset();
            if (onReset)
                onReset();
        }
        else
        {
            // the lock first, so divisions really returns to its default
            setWithGesture (pDivLock, spec (pDivLock).def);
            for (int i = 0; i < numParams; ++i)
                setWithGesture (i, spec (i).def);
        }
        return;
    }

    const auto* row = rowAt (position);
    if (row == nullptr)
        return;

    if (row->type == RowType::toggle)
    {
        setFromUI (row->param, processor.toggle (row->param) ? 0.0 : 1.0);
        return;
    }

    const ParamSpec& p = spec (row->param);
    const auto track = trackBounds (*row);

    if (row->param == pDivisions && ! processor.toggle (pDivLock))
        setWithGesture (pDivLock, 1.0);

    dragParam = row->param;
    dragLastX = position.getX();
    processor.parameter (dragParam)->beginChangeGesture();

    // Shift grabs the value where it is, for a fine drag; a plain click jumps
    if (event.getModifiers().isShiftDown())
    {
        dragNorm = p.toNorm (shownValue (dragParam));
        return;
    }

    dragNorm = std::clamp (static_cast<double> ((position.getX() - track.getX()) / track.getWidth()), 0.0, 1.0);
    setWithGesture (dragParam, p.fromNorm (dragNorm));
    repaint();
}

void ParameterPanel::mouseDrag (const yup::MouseEvent& event)
{
    if (dragParam < 0)
        return;

    const auto& row = *std::find_if (rows.begin(), rows.end(), [this] (const Row& r) { return r.param == dragParam; });
    const auto track = trackBounds (row);
    const float x = event.getPosition().getX();

    if (event.getModifiers().isShiftDown())
        dragNorm = std::clamp (dragNorm + static_cast<double> ((x - dragLastX) / track.getWidth()) * 0.1, 0.0, 1.0);
    else
        dragNorm = std::clamp (static_cast<double> ((x - track.getX()) / track.getWidth()), 0.0, 1.0);

    dragLastX = x;
    setWithGesture (dragParam, spec (dragParam).fromNorm (dragNorm));
    repaint();
}

void ParameterPanel::mouseUp (const yup::MouseEvent&)
{
    if (dragParam < 0)
        return;

    processor.parameter (dragParam)->endChangeGesture();
    dragParam = -1;
    repaint();
}

void ParameterPanel::mouseDoubleClick (const yup::MouseEvent& event)
{
    if (const auto* row = rowAt (event.getPosition()))
        setFromUI (row->param, spec (row->param).def);
}

} // namespace phyllotaxis
