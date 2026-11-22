#pragma once

#include "ParameterPanel.h"
#include "PatternView.h"
#include "PhyllotaxisProcessor.h"

#include <yup_audio_processors/yup_audio_processors.h>

namespace phyllotaxis
{

/** The spiral on the left, the collapsible parameter panel on the right. */
class PhyllotaxisEditor final : public yup::AudioProcessorEditor
{
public:
    explicit PhyllotaxisEditor (PhyllotaxisProcessor& processor);

    bool isResizable() const override { return true; }
    yup::Size<int> getPreferredSize() const override { return { 900, 660 }; }

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    /** The panel needs room for itself and a usable canvas. */
    bool panelFits() const;

    PhyllotaxisProcessor& processor;
    PatternView patternView;
    ParameterPanel parameterPanel;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhyllotaxisEditor)
};

} // namespace phyllotaxis
