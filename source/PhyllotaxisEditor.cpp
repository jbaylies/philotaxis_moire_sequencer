#include "PhyllotaxisEditor.h"
#include "PhyllotaxisTheme.h"

namespace phyllotaxis
{

PhyllotaxisEditor::PhyllotaxisEditor (PhyllotaxisProcessor& p)
    : processor (p)
    , patternView (p)
    , parameterPanel (p)
{
    setOpaque (true);

    patternView.onPanelButton = [this]
    {
        processor.setPanelVisible (! processor.isPanelVisible());
        resized();
    };

    parameterPanel.onReset = [this] { patternView.clearFlashes(); };

    addAndMakeVisible (patternView);
    addChildComponent (parameterPanel);
}

bool PhyllotaxisEditor::panelFits() const
{
    return getWidth() >= ParameterPanel::width + 120.0f;
}

void PhyllotaxisEditor::resized()
{
    auto bounds = getLocalBounds();
    const bool showPanel = processor.isPanelVisible() && panelFits();

    parameterPanel.setVisible (showPanel);
    if (showPanel)
        parameterPanel.setBounds (bounds.removeFromRight (ParameterPanel::width));

    patternView.setBounds (bounds);
    patternView.setPanelShown (showPanel);
}

void PhyllotaxisEditor::paint (yup::Graphics& g)
{
    g.setFillColor (theme::canvas);
    g.fillAll();
}

//==============================================================================
yup::AudioProcessorEditor* PhyllotaxisProcessor::createEditor()
{
    return new PhyllotaxisEditor (*this);
}

} // namespace phyllotaxis
