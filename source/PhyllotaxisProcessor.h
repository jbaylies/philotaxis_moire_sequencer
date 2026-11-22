#pragma once

#include "PhyllotaxisParameters.h"
#include "PhyllotaxisPattern.h"

#include <yup_audio_processors/yup_audio_processors.h>

#include <atomic>

namespace phyllotaxis
{

/** Single producer, single consumer queue of triggered site indices. The audio
    thread pushes, the editor pops; when no editor drains it, pushes are dropped. */
class FlashQueue
{
public:
    struct Flash
    {
        int site = 0;
        yup::int64 time = 0; // audio clock, in samples
    };

    void push (const Flash& flash) noexcept;
    bool pop (Flash& flash) noexcept;

private:
    static constexpr yup::uint32 capacity = 1u << 14;
    std::array<Flash, capacity> items {};
    std::atomic<yup::uint32> writeIndex { 0 };
    std::atomic<yup::uint32> readIndex { 0 };
};

//==============================================================================
class PhyllotaxisProcessor final : public yup::AudioProcessor
{
public:
    PhyllotaxisProcessor();

    /** Real value of a parameter, snapped to its step in double precision. */
    double param (int index) const;
    bool toggle (int index) const { return param (index) >= 0.5; }
    const yup::AudioParameter::Ptr& parameter (int index) const { return params[static_cast<size_t> (index)]; }

    /** What the audio thread last published for the editor. */
    struct View
    {
        double rotPhase = 0.0;
        double palPhase = 0.0;
        double divisions = 56.0;
        double discriminant = 5.0;
        double linePhase = 0.25;
        bool running = false;
        yup::int64 clock = 0;
        double sampleRate = 44100.0;
        int lastPitch = -1;
        int lastVelocity = 0;
        yup::int64 lastNoteTime = 0;
    };

    View getView() const;
    FlashQueue& getFlashQueue() noexcept { return flashes; }

    /** Line back to the top and phases to zero, done by the next block. */
    void requestReset() noexcept { resetRequested.store (true); }

    bool isPanelVisible() const noexcept { return panelVisible.load(); }
    void setPanelVisible (bool visible) noexcept { panelVisible.store (visible); }

    //==============================================================================
    void prepareToPlay (const yup::AudioSpec& spec) override;
    void releaseResources() override {}
    void processBlock (yup::AudioProcessContext<float>& context) override;
    void flush() override { flushRequested.store (true); }

    bool hasEditor() const override { return true; }
    yup::AudioProcessorEditor* createEditor() override;

    int getCurrentPreset() const noexcept override { return 0; }
    void setCurrentPreset (int) noexcept override {}
    int getNumPresets() const override { return 0; }
    yup::String getPresetName (int) const override { return {}; }
    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }
    yup::Result saveStateIntoDataTree (yup::DataTree& state) override;
    yup::Result loadStateFromDataTree (const yup::DataTree& state) override;

private:
    struct Hit
    {
        int pitch = 0;
        int velocity = 0;
        double position = 0.0; // 0..1 through the step
    };

    PatternMotion::Targets motionTargets() const;
    void stepAndSweep (yup::MidiBuffer& midi, int start, int length);
    void sweepLine (double from, double to, int stepStart, int stepLength);
    void noteOn (yup::MidiBuffer& midi, int pitch, int velocity, int offset);
    void flushNotes (yup::MidiBuffer& midi, int offset);
    void publish();

    static constexpr int maxNotesPerTick = 32;

    std::array<yup::AudioParameter::Ptr, numParams> params;

    // Audio thread state
    PatternMotion motion;
    Sites sites;
    Palette palette;
    double linePhase = 0.25;
    double sampleRate = 44100.0;
    int tickLength = 220;
    int samplesIntoTick = 0;
    yup::int64 clock = 0;
    yup::int64 blockClock = 0;
    int activeChannel = 0;
    bool wasRunning = false;

    std::array<Hit, maxPoints> hits {};
    int numHits = 0;
    std::array<int, 128> loudestHit {};

    std::array<bool, 128> noteActive {};
    std::array<yup::int64, 128> noteEnd {};
    int noteChannel = 1;

    // Shared with the editor
    std::atomic<double> viewRotPhase { 0.0 };
    std::atomic<double> viewPalPhase { 0.0 };
    std::atomic<double> viewDivisions { 56.0 };
    std::atomic<double> viewDiscriminant { 5.0 };
    std::atomic<double> viewLinePhase { 0.25 };
    std::atomic<double> viewSampleRate { 44100.0 };
    std::atomic<yup::int64> viewClock { 0 };
    std::atomic<bool> viewRunning { false };
    std::atomic<int> lastNote { -1 }; // pitch << 8 | velocity
    std::atomic<yup::int64> lastNoteTime { 0 };

    std::atomic<bool> resetRequested { false };
    std::atomic<bool> flushRequested { false };
    std::atomic<bool> panelVisible { true };

    FlashQueue flashes;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhyllotaxisProcessor)
};

} // namespace phyllotaxis
