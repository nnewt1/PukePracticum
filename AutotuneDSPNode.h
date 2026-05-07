/// @file src/nodes/dsp/AutotuneDSPNode.h
/// @brief DSP adapter for the Autotune node.
///
/// Composes puke::dsp::PitchDetect (MPM/NSDF tracker) with
/// puke::dsp::Harmonizer (windowed delay-head pitch shifter).
///
/// Per block:
///   1. PitchDetect gives us the current fundamental in Hz.
///   2. We quantise Hz -> nearest in-scale MIDI note -> semitone correction.
///   3. Harmonizer voice 0 shifts by that amount; voices 1 & 2 are silenced.
///   4. Wet/dry blend applied after.
///
/// Output port layout (mirrors Harmonizer, required to avoid OOB reads):
///   outputs[0..nCh-1]          main (corrected audio)
///   outputs[nCh..2nCh-1]       dry pass-through
///   outputs[2nCh..3nCh-1]      voice 1 out (unused, gain=0)
///   outputs[3nCh..4nCh-1]      voice 2 out (unused, gain=0)
///   outputs[4nCh..5nCh-1]      voice 3 out (unused, gain=0)

#pragma once
#include "DSPNode.h"

#include <juce_core/juce_core.h>
#include <puke_dsp/harmonizer.h>
#include <puke_dsp/limits.h>
#include <puke_dsp/pitch_detect.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace autotune_detail {

    inline bool noteInScale(int note, int mask) noexcept {
        return (mask >> (note & 11)) & 1;
    }

    inline float computeCorrectionSemitones(float detectedHz, int key, int scaleMask) noexcept {
        if (detectedHz <= 0.0f)
            return 0.0f;

        const float detectedMidi = 69.0f + 12.0f * std::log2(detectedHz / 440.0f);
        const int   centerMidi = (int)std::round(detectedMidi);

        float bestDist = 999.0f;
        int   bestMidi = centerMidi;

        for (int offset = -12; offset <= 12; ++offset) {
            const int candidate = centerMidi + offset;
            const int pitchClass = ((candidate % 12) + 12 - key) % 12;
            if (!noteInScale(pitchClass, scaleMask))
                continue;
            const float dist = std::abs((float)candidate - detectedMidi);
            if (dist < bestDist) {
                bestDist = dist;
                bestMidi = candidate;
            }
        }

        return std::max(-12.0f, std::min(12.0f, (float)(bestMidi - centerMidi)));
    }

    static constexpr int kMaskChromatic = 0b111111111111;
    static constexpr int kMaskMajor = 0b101011010101;
    static constexpr int kMaskMinor = 0b010110101101;

} // namespace autotune_detail

/// DSP adapter for the Autotune node.
struct AutotuneDSPNode : public DSPNode {

    // --- PitchDetect split storage ------------------------------------------
    puke::dsp::PitchDetect             detector;
    puke::dsp::PitchDetectHotStorage   detectorHot{};
    puke::dsp::PitchDetectStateStorage detectorState{};
    puke::dsp::PitchDetectRingStorage  detectorRing{};

    // --- Harmonizer ---------------------------------------------------------
    puke::dsp::Harmonizer shifter;

    // --- Scratch / pointer arrays -------------------------------------------
    static constexpr int kMaxScratch = PUKE_DSP_MAX_FFT_SIZE; // 4096
    static constexpr int kMaxChans = PUKE_DSP_MAX_CHANNELS;
    static constexpr int kHarmInGroups = 12; // audio-in + 11 mod ports
    static constexpr int kHarmOutGroups = 5;  // main + dry + voice1/2/3

    float hzScratch[kMaxScratch]{};

    std::vector<float>        extraOutBuffers; // backing store for out groups 1-4
    std::vector<const float*> shifterInPtrs;   // kHarmInGroups  * numChans
    std::vector<float*>       shifterOutPtrs;  // kHarmOutGroups * numChans

    // --- Parameters ---------------------------------------------------------
    int   key = 0;
    int   scale = 0;
    float speed = 25.0f;
    float mix = 1.0f;
    int   numChans = 1;

    AutotuneDSPNode() {
        detector.bindStorage(&detectorHot, &detectorState, &detectorRing);
        detector.setUseDaisyBackendOnDesktop(true);
    }

    // ------------------------------------------------------------------------
    void prepare(double sr, int blockSize, int nChans) override {
        numChans = std::max(1, std::min(nChans, kMaxChans));

        detector.bindStorage(&detectorHot, &detectorState, &detectorRing);
        detector.setUseDaisyBackendOnDesktop(true);
        detector.prepare(sr, numChans);

        shifter.semitones[0] = 0.0f;
        shifter.semitones[1] = 0.0f;
        shifter.semitones[2] = 0.0f;
        shifter.gains[0] = 1.0f;
        shifter.gains[1] = 0.0f;
        shifter.gains[2] = 0.0f;
        shifter.dry = 0.0f;
        shifter.mix = 1.0f;
        shifter.level = 1.0f;
        shifter.slewMs = speed;
        shifter.prepare(sr, numChans);

        // 4 extra output groups (dry, voice1, voice2, voice3), each blockSize
        // samples per channel.
        const int safeScratch = std::max(1, blockSize);
        extraOutBuffers.assign((size_t)(4 * numChans * safeScratch), 0.0f);

        // Pre-size pointer arrays; filled every process() call.
        shifterInPtrs.assign((size_t)(kHarmInGroups * numChans), nullptr);
        shifterOutPtrs.assign((size_t)(kHarmOutGroups * numChans), nullptr);
    }

    // ------------------------------------------------------------------------
    void setParam(const juce::String& id, float value) override {
        if (id == "key")
            key = std::max(0, std::min(11, (int)std::lround(value)));
        else if (id == "scale")
            scale = std::max(0, std::min(2, (int)std::lround(value)));
        else if (id == "speed") {
            speed = juce::jlimit(1.0f, 250.0f, value);
            shifter.slewMs = speed;
        }
        else if (id == "mix")
            mix = juce::jlimit(0.0f, 1.0f, value);
        else if (id == "window")
            detector.setParam("window", value);
    }

    // ------------------------------------------------------------------------
    void process(const float* const* inputs,
        float* const* outputs,
        int                 numSamples) override {
        if (outputs == nullptr || numSamples <= 0)
            return;

        const int safe = std::min(numSamples, kMaxScratch);

        // 1. Pitch detection -------------------------------------------------
        const float* detIn[1] = { inputs ? inputs[0] : nullptr };
        float* detOut[1] = { hzScratch };
        detector.process(detIn, detOut, safe);

        // 2. Quantise to scale -----------------------------------------------
        const float detectedHz = hzScratch[safe - 1];
        const int mask = (scale == 1) ? autotune_detail::kMaskMajor
            : (scale == 2) ? autotune_detail::kMaskMinor
            : autotune_detail::kMaskChromatic;
        shifter.semitones[0] =
            autotune_detail::computeCorrectionSemitones(detectedHz, key, mask);

        // 3. Build input pointer array (12 * numChans) -----------------------
        // Group 0 = audio in. Groups 1-11 = nullptr so Harmonizer falls back
        // to its stored param values. No zeroBuffer — nullptr is correct here.
        std::fill(shifterInPtrs.begin(), shifterInPtrs.end(), nullptr);
        if (inputs != nullptr) {
            for (int ch = 0; ch < numChans; ++ch)
                shifterInPtrs[(size_t)ch] = inputs[ch];
        }

        // 4. Build output pointer array (5 * numChans) -----------------------
        // Group 0 = main output (what users wire to Audio Out).
        // Groups 1-4 = scratch buffers we own; stride derived from allocation
        // in prepare() so it never exceeds the buffer size.
        const int stride =
            (int)(extraOutBuffers.size() / std::max(1, 4 * numChans));

        for (int ch = 0; ch < numChans; ++ch)
            shifterOutPtrs[(size_t)ch] = outputs[ch];

        for (int slot = 1; slot < kHarmOutGroups; ++slot) {
            for (int ch = 0; ch < numChans; ++ch) {
                const int idx = (slot - 1) * numChans + ch;
                shifterOutPtrs[(size_t)(slot * numChans + ch)] =
                    extraOutBuffers.data() + (size_t)(idx * stride);
            }
        }

        // 5. Pitch shift -----------------------------------------------------
        shifter.process(shifterInPtrs.data(), shifterOutPtrs.data(), numSamples);

        // 6. Wet/dry blend ---------------------------------------------------
        if (mix < 1.0f && inputs != nullptr) {
            for (int ch = 0; ch < numChans; ++ch) {
                const float* in = inputs[ch];
                float* out = outputs[ch];
                if (in == nullptr || out == nullptr) continue;
                for (int i = 0; i < numSamples; ++i)
                    out[i] = in[i] * (1.0f - mix) + out[i] * mix;
            }
        }
    }
};
