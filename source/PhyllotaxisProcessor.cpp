#include "PhyllotaxisProcessor.h"

#include <algorithm>
#include <cmath>

namespace phyllotaxis
{

//==============================================================================
void FlashQueue::push (const Flash& flash) noexcept
{
    const auto write = writeIndex.load (std::memory_order_relaxed);
    if (write - readIndex.load (std::memory_order_acquire) >= capacity)
        return;

    items[write % capacity] = flash;
    writeIndex.store (write + 1, std::memory_order_release);
}

bool FlashQueue::pop (Flash& flash) noexcept
{
    const auto read = readIndex.load (std::memory_order_relaxed);
    if (read == writeIndex.load (std::memory_order_acquire))
        return false;

    flash = items[read % capacity];
    readIndex.store (read + 1, std::memory_order_release);
    return true;
}

//==============================================================================
namespace
{
yup::AudioBusLayout makeBusLayout()
{
    using Bus = yup::AudioBus;
    return yup::AudioBusLayout ({ Bus ("midi in", Bus::Midi, Bus::Input, 0) },
                                { Bus ("output", Bus::Audio, Bus::Output, 2),
                                  Bus ("midi out", Bus::Midi, Bus::Output, 0) });
}
} // namespace

PhyllotaxisProcessor::PhyllotaxisProcessor()
    : yup::AudioProcessor ("Phyllotaxis", makeBusLayout())
{
    for (int i = 0; i < numParams; ++i)
    {
        params[static_cast<size_t> (i)] = createParameter (i);
        addParameter (params[static_cast<size_t> (i)]);
    }

    motion = PatternMotion (motionTargets());
    sites.update (static_cast<int> (param (pPoints)), param (pDiscriminant));
}

double PhyllotaxisProcessor::param (int index) const
{
    return spec (index).quantize (static_cast<double> (params[static_cast<size_t> (index)]->getValue()));
}

PatternMotion::Targets PhyllotaxisProcessor::motionTargets() const
{
    return { param (pSpeed), param (pDivisions), toggle (pDivLock), param (pDiscriminant) };
}

//==============================================================================
void PhyllotaxisProcessor::prepareToPlay (const yup::AudioSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0f ? static_cast<double> (spec.sampleRate) : 44100.0;
    tickLength = std::max (1, static_cast<int> (std::lround (sampleRate * 0.005))); // the original's 5 ms task
    samplesIntoTick = 0;
    viewSampleRate.store (sampleRate);
}

void PhyllotaxisProcessor::processBlock (yup::AudioProcessContext<float>& context)
{
    auto& midi = context.midi;
    midi.clear();
    context.audio.clear();

    const int numSamples = context.audio.getNumSamples();
    blockClock = clock;

    if (flushRequested.exchange (false))
        flushNotes (midi, 0);

    if (const int channel = static_cast<int> (param (pChannel)); channel != noteChannel)
    {
        flushNotes (midi, 0);
        noteChannel = channel;
    }

    bool hostPlaying = true;
    if (toggle (pFollowTransport) && context.playHead != nullptr)
    {
        if (const auto position = context.playHead->getPosition())
            hostPlaying = position->getIsPlaying();
    }

    const bool running = toggle (pPlay) && hostPlaying;
    if (wasRunning && ! running)
        flushNotes (midi, 0);
    wasRunning = running;

    if (resetRequested.exchange (false))
    {
        linePhase = 0.25;
        motion.resetPhases();
        flushNotes (midi, 0);
    }

    for (int start = 0; start < numSamples;)
    {
        const int length = std::min (numSamples - start, tickLength - samplesIntoTick);
        stepAndSweep (midi, start, length);
        start += length;
        samplesIntoTick = (samplesIntoTick + length) % tickLength;
    }

    clock += numSamples;
    publish();
}

//==============================================================================
void PhyllotaxisProcessor::stepAndSweep (yup::MidiBuffer& midi, int start, int length)
{
    const double dt = length / sampleRate;

    motion.step (dt, motionTargets());
    sites.update (static_cast<int> (param (pPoints)), motion.getDiscriminant());
    palette.update ({ param (pHue), param (pHueSpan), param (pLightness), param (pLightSwing), param (pChroma) });

    if (wasRunning)
    {
        const double from = linePhase;
        const double to = linePhase - (param (pRpm) / 60.0) * dt; // positive rpm = clockwise
        sweepLine (from, to, start, length);
        linePhase = fract (to);

        // one note per pitch per tick (the loudest), the loudest first, capped
        for (int i = 0; i < std::min (numHits, maxNotesPerTick); ++i)
        {
            const Hit& hit = hits[static_cast<size_t> (i)];
            const int offset = start + std::min (length - 1, static_cast<int> (hit.position * length));
            noteOn (midi, hit.pitch, hit.velocity, offset);
            lastNote.store ((hit.pitch << 8) | hit.velocity);
            lastNoteTime.store (blockClock + offset);
        }
    }

    const auto stepEnd = blockClock + start + length;
    for (size_t p = 0; p < noteActive.size(); ++p)
    {
        if (noteActive[p] && noteEnd[p] < stepEnd)
        {
            const auto offset = static_cast<int> (std::max (noteEnd[p] - blockClock, static_cast<yup::int64> (start)));
            midi.addEvent (yup::MidiMessage::noteOff (noteChannel, static_cast<int> (p)), offset);
            noteActive[p] = false;
        }
    }
}

// Collects every site the line swept over going from `from` to `to` (turns),
// keeping the loudest hit of each pitch in the order the pitches first appeared
void PhyllotaxisProcessor::sweepLine (double from, double to, int stepStart, int stepLength)
{
    numHits = 0;

    const double delta = std::abs (to - from);
    if (delta <= 0.0)
        return;

    loudestHit.fill (-1);

    const bool clockwise = to < from;
    const ColorFrame frame { motion.getDivisions(), motion.getRotPhase(), motion.getPalPhase(), toggle (pMaskType) };
    const double gate = param (pGate);
    const double noteMin = param (pNoteMin);
    const double noteMax = param (pNoteMax);

    for (int n = 0; n < sites.size(); ++n)
    {
        // (from, to]: d is how far past `from` the site sits, in the line's direction
        const double d = clockwise ? fract (from - sites.angle (n)) : fract (sites.angle (n) - from);
        if (! (delta >= 1.0 || (d > 0.0 && d <= delta)))
            continue;

        const auto velocity = static_cast<int> (std::round (brightness (siteColor (n, sites, palette, frame)) * 127.0));
        if (velocity < 1 || velocity < gate) // gated points get no flash either
            continue;

        const double position = delta >= 1.0 ? 0.0 : d / delta;
        flashes.push ({ n, blockClock + stepStart + std::min (stepLength - 1, static_cast<int> (position * stepLength)) });

        const auto pitch = static_cast<int> (std::round (noteMin + (noteMax - noteMin) * sites.radius (n)));
        auto& slot = loudestHit[static_cast<size_t> (pitch)];

        if (slot < 0)
        {
            slot = numHits++;
            hits[static_cast<size_t> (slot)] = { pitch, velocity, position };
        }
        else if (velocity > hits[static_cast<size_t> (slot)].velocity)
        {
            hits[static_cast<size_t> (slot)] = { pitch, velocity, position };
        }
    }

    // stable for equal velocities without std::stable_sort, which may allocate:
    // hits are already in first-appearance order, so sort by (velocity, index)
    for (int i = 0; i < numHits; ++i)
        hits[static_cast<size_t> (i)].velocity = hits[static_cast<size_t> (i)].velocity * 16384 + (16383 - i);

    std::sort (hits.begin(), hits.begin() + numHits, [] (const Hit& a, const Hit& b) { return a.velocity > b.velocity; });

    for (int i = 0; i < numHits; ++i)
        hits[static_cast<size_t> (i)].velocity /= 16384;
}

void PhyllotaxisProcessor::noteOn (yup::MidiBuffer& midi, int pitch, int velocity, int offset)
{
    const auto p = static_cast<size_t> (pitch);

    // retrigger: end the sounding note first, so every note-on has its own note-off
    if (noteActive[p])
    {
        const auto end = static_cast<int> (std::clamp (noteEnd[p] - blockClock, static_cast<yup::int64> (0), static_cast<yup::int64> (offset)));
        midi.addEvent (yup::MidiMessage::noteOff (noteChannel, pitch), end);
    }

    midi.addEvent (yup::MidiMessage::noteOn (noteChannel, pitch, static_cast<yup::uint8> (velocity)), offset);

    const auto length = std::max<yup::int64> (1, std::llround (param (pDuration) * sampleRate / 1000.0));
    noteActive[p] = true;
    noteEnd[p] = blockClock + offset + length;
}

void PhyllotaxisProcessor::flushNotes (yup::MidiBuffer& midi, int offset)
{
    for (size_t p = 0; p < noteActive.size(); ++p)
    {
        if (noteActive[p])
            midi.addEvent (yup::MidiMessage::noteOff (noteChannel, static_cast<int> (p)), offset);
    }

    noteActive.fill (false);
}

//==============================================================================
void PhyllotaxisProcessor::publish()
{
    viewRotPhase.store (motion.getRotPhase(), std::memory_order_relaxed);
    viewPalPhase.store (motion.getPalPhase(), std::memory_order_relaxed);
    viewDivisions.store (motion.getDivisions(), std::memory_order_relaxed);
    viewDiscriminant.store (motion.getDiscriminant(), std::memory_order_relaxed);
    viewLinePhase.store (linePhase, std::memory_order_relaxed);
    viewRunning.store (wasRunning, std::memory_order_relaxed);
    viewClock.store (clock, std::memory_order_release);
}

PhyllotaxisProcessor::View PhyllotaxisProcessor::getView() const
{
    View v;
    v.clock = viewClock.load (std::memory_order_acquire);
    v.rotPhase = viewRotPhase.load (std::memory_order_relaxed);
    v.palPhase = viewPalPhase.load (std::memory_order_relaxed);
    v.divisions = viewDivisions.load (std::memory_order_relaxed);
    v.discriminant = viewDiscriminant.load (std::memory_order_relaxed);
    v.linePhase = viewLinePhase.load (std::memory_order_relaxed);
    v.running = viewRunning.load (std::memory_order_relaxed);
    v.sampleRate = viewSampleRate.load (std::memory_order_relaxed);

    if (const int note = lastNote.load(); note >= 0)
    {
        v.lastPitch = note >> 8;
        v.lastVelocity = note & 0xff;
        v.lastNoteTime = lastNoteTime.load();
    }

    return v;
}

//==============================================================================
yup::Result PhyllotaxisProcessor::saveStateIntoDataTree (yup::DataTree& state)
{
    state = yup::DataTree ("PhyllotaxisState");
    auto transaction = state.beginTransaction();
    transaction.setProperty ("version", 1);
    transaction.setProperty ("panel", isPanelVisible());

    for (const auto& p : params)
        transaction.setProperty (p->getID(), static_cast<double> (p->getValue()));

    return yup::Result::ok();
}

yup::Result PhyllotaxisProcessor::loadStateFromDataTree (const yup::DataTree& state)
{
    if (state.getType() != yup::Identifier ("PhyllotaxisState"))
        return yup::Result::fail ("Not a Phyllotaxis state");

    for (const auto& p : params)
        p->setValue (static_cast<float> (state.getProperty (p->getID(), static_cast<double> (p->getDefaultValue()))));

    setPanelVisible (static_cast<bool> (state.getProperty ("panel", true)));
    updateHostDisplay (ChangeDetails().withParameterValuesChanged (true));
    return yup::Result::ok();
}

} // namespace phyllotaxis

extern "C" yup::AudioProcessor* createPluginProcessor()
{
    return new phyllotaxis::PhyllotaxisProcessor();
}
