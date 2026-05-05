#pragma once
// PUKE_CUSTOM_TEMPLATE
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include "nodes/custom/CustomCppModuleAPI.h"
#include <puke_dsp/delay_ring.h>
#include <puke_dsp/limits.h>

#if defined(PUKE_CUSTOM_MODULE) && !defined(PUKE_CUSTOM_STATIC)
    #define PUKE_CUSTOM_STATIC 1
#endif

#if !defined(PUKE_CUSTOM_NAMESPACE)
    #define PUKE_CUSTOM_NAMESPACE puke::custom_cpp
#endif

#if !defined(PUKE_CUSTOM_TYPE_ID)
    #define PUKE_CUSTOM_TYPE_ID "custom_cpp"
#endif

namespace PUKE_CUSTOM_NAMESPACE
{
//-------------------------------------------------------------------------
// Custom C++ node template (delay line with feedback).
//
// This file is the entire "node definition" AND the DSP code. Students can:
// 1) Change metadata: label/category/description.
// 2) Define UI params in kUserParams (sliders, buttons, text).
// 3) Add signal ports in kUserPorts (extra audio-rate I/O).
// 4) Implement DSP in Processor::process() (your audio code goes there).
//
// Port rules:
// - The engine always provides the default audio In and Out (port 0).
// - Extra signal ports are appended after that and appear in the inputs array.
//
// Safety rule:
// - If you change the number/order of signal ports, re-add the node or reload
//   the patch so the graph recompiles with the new layout.
//
// Reference (template constants + flags):
// - PUKE_SIMPLE_DELAY_MAX_DELAY_SAMPLES: maximum delay length (samples) for
//   simple delay lines. Defined in puke_dsp/limits.h.
// - PUKE_CUSTOM_PARAM_USE_SLIDER: UI flag to render a numeric parameter as a
//   slider/knob (often combined with EXPOSED_BY_DEFAULT).
//-------------------------------------------------------------------------
static constexpr const char* kNodeTypeId        = PUKE_CUSTOM_TYPE_ID;
static constexpr const char* kNodeLabel         = "psola";
static constexpr const char* kNodeDescription   = "Delay with audio-rate delay, feedback, and master gain.";
static constexpr const char* kNodeCategory      = "Custom";
static constexpr const char* kNodeSubcategory   = "DSP";

// Params define UI controls and optional param ports.
// To add a new UI control, append a new entry here and handle it in setParam().
static const std::array<PukeCustomParamDesc, 1> kUserParams = {{
    { "shift", "Shift", PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE,
      -2, 2, 0.0, nullptr, nullptr, 0,
      PUKE_CUSTOM_PARAM_USE_SLIDER | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT }
}};

// Signal ports are audio-rate and show up in inputs/outputs arrays.
// To add a port, append here and update the offsets in process().
// Port indices are packed by port (mono):
//   inputs[0] -> default In
//   inputs[1] -> first extra port ("gain")
//   inputs[2] -> second extra port ("delay_samples")
//   inputs[3] -> third extra port ("feedback")
static const std::array<PukeCustomPortDesc, 1> kUserPorts = {{
    { "pitch", "Pitch In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN }
}};

inline const PukeCustomNodeMetadata* getCustomNodeMetadata()
{
    static const PukeCustomNodeMetadata meta
    {
        kPukeCustomMetadataVersion,
        kNodeTypeId,
        kNodeLabel,
        kNodeDescription,
        kNodeCategory,
        kNodeSubcategory,
        kUserPorts.data(),
        (int) kUserPorts.size(),
        kUserParams.data(),
        (int) kUserParams.size()
    };
    return &meta;
}

class Processor
{
public:
    void prepare (double sr, int /*blockSize*/, int nChans)
    {
        sampleRate = sr > 0.0 ? sr : 48000.0;
        numChans = std::max (1, nChans);
        const int maxDelay = std::max (1, (int) PUKE_SIMPLE_DELAY_MAX_DELAY_SAMPLES);
        rings.resize ((size_t) numChans);
        for (int ch = 0; ch < numChans; ++ch)
            rings[(size_t) ch].prepare (maxDelay);
    }

    void setParam (const char* id, float value)
    {
        if (id == nullptr)
            return;
        if (id, "shift")
            shift = value;
    }

    std::vector<int> findPeaks (int numSamples, const float* in, const float freq) {
        std::vector<int> peaks;
        if (freq < 10 || freq > 10000) { // freq is out of bounds
            return peaks;
        }

        float period = sampleRate/freq;
        if (period > numSamples) {
            return peaks;
        }

        if (period < 1.0f) period = 1.0f;

        int cnt = 0;
        for (float t = period; t < numSamples - period/2; t+=period) {
            peaks.push_back(t);
            cnt += 1;
        }

        return peaks;
    }

    std::vector<int> generatePeaks (std::vector<int> peaks, float scalingFactor) {
        int numPeaks = peaks.size();
        int numNewPeaks = float(numPeaks) * scalingFactor;

        std::vector<int> newPeaks(numNewPeaks);

        // No peaks
        if (numPeaks <= 0 || numNewPeaks <= 0) return newPeaks;

        if (numNewPeaks == 1) newPeaks[0] = 0.0f;
        else {
            for (int i = 0; i < numNewPeaks; i++) {
                float ref = float(i) * (numPeaks - 1.0f) / (numNewPeaks - 1.0f);
                float weight = ref - std::floor(ref);
                int left = std::floor(ref);
                int right = std::ceil(ref);

                if (left > numPeaks - 1)
                    left = numPeaks - 1;
                if (right > numPeaks - 1)
                    right = numPeaks - 1;
                newPeaks[i] = (peaks[left] * (1.0f - weight)) + (peaks[right] * weight);
            }
        }

        return newPeaks;
    }

    void pitchShift2(int numSamples, const float* in, float* out, float freq, float scalingFactor) {
        std::vector<int> peaks = findPeaks(numSamples, in, freq);
        std::vector<int> newPeaks = generatePeaks(peaks, scalingFactor);
        int numPeaks = peaks.size();
        int numNewPeaks = newPeaks.size();

        // PSOLA
        for (int p = 0; p < numNewPeaks; p++) {
            const int dstPeak = newPeaks[p];

            // Find closest original peak
            int best_i = 0;
            int best_dist = std::abs(peaks[0] - dstPeak);
            for (int k = 1; k < numPeaks; k++) {
                int d = std::abs(peaks[k] - dstPeak);
                if (d < best_dist) {
                    best_dist = d;
                    best_i = k;
                }
            }
            const int srcPeak = peaks[best_i]; // Original peak to source from

            // Window half-widths: distance to adjacent NEW peaks
            int p_left = (p == 0) ? dstPeak : dstPeak - newPeaks[p - 1];
            int p_right = (p == numNewPeaks - 1) ? numSamples - 1 - dstPeak : newPeaks[p + 1] - dstPeak;

            // Clamp values
            if (srcPeak - p_left < 0)               p_left = srcPeak;
            if (srcPeak + p_right > numSamples - 1) p_right = numSamples - 1 - srcPeak;
            if (dstPeak - p_left < 0)               p_left = dstPeak;
            if (dstPeak + p_right > numSamples - 1) p_right = numSamples - 1 - dstPeak;

            if (p_left <= 0 && p_right <= 0) continue;

            // Build triangular window  [0→1] over p_left samples, [1→0] over p_right
            const int win_size = p_left + p_right;
            std::vector<float> window(win_size);

            // Rising ramp: indices [0, p_left-1]  →  values (0, 1]
            for (int k = 0; k < p_left; k++)
                window[k] = static_cast<float>(k + 1) / static_cast<float>(p_left + 1);

            // Falling ramp: indices [p_left, win_size-1]  →  values [1, 0)
            for (int k = 0; k < p_right; k++)
                window[p_left + k] = static_cast<float>(p_right - k) /
                                    static_cast<float>(p_right + 1);

            // Overlap-add: output[nj - p_left : nj + p_right] += window * signal[pi - p_left : pi + p_right]
            const int out_start = dstPeak - p_left;
            const int src_start = srcPeak - p_left;
            for (int k = 0; k < win_size; k++)
                out[out_start + k] += window[k] * in[src_start + k];
        }
}

    float windowValue(int windowLength, int windowIdx) {
        if (windowIdx <= 0 || windowIdx >= windowLength) return 0.0f;
        if (windowIdx < windowLength / 2) {
            int offset = windowIdx - 1;
            float slope = 1.0f / (windowLength - 2.0);
            return offset * slope;
        }
        else {
            int offset = windowLength - windowIdx - 2;
            float slope = 1.0f / (windowLength - 2.0);
            return offset * slope;
        }

    }

    void pitchShift(int numSamples, const float* in, float* out, float freq, float scalingFactor) {
        int srcShift = ceil(sampleRate / freq);
        int halfSrcShift = ceil(srcShift / 2);

        int dstShift = round(srcShift * scalingFactor);

        int srcIdx = -1;
        int dstIdx = 0;

        int srcBlockStart, srcBlockEnd;
        int dstBlockEnd;

        int srcLimit = numSamples - srcShift - 1;

        int windowLength = srcShift + halfSrcShift + 1;

        while (srcIdx < srcLimit) {
            srcBlockStart = (srcIdx + 1) - halfSrcShift;
            if (srcBlockStart < 0) srcBlockStart = 0; // Clamp

            srcBlockEnd = srcBlockStart + srcShift + halfSrcShift;
            if (srcBlockEnd > numSamples - 1) srcBlockEnd = numSamples - 1; // Clamp

            // Overlap and Add
            dstBlockEnd = dstIdx + srcBlockEnd - srcBlockStart;

            int inputIdx = srcBlockStart;
            int windowIdx = 0;
            for (int j = dstIdx; j <= dstBlockEnd; j++)
            {
                if (j >= numSamples) return;
                out[j] += in[inputIdx] * windowValue(windowLength, windowIdx);
                inputIdx++;
                windowIdx++;
            }

            srcIdx += srcShift;
            dstIdx += dstShift;
        }
    }

    void process (const float* const* inputs,
                  float* const* outputs,
                  int numSamples)
    {
        // ======================
        // YOUR DSP ROUTINE HERE
        // ======================
        if (outputs == nullptr || numSamples <= 0)
            return;

        // Each extra port takes one slot in the inputs array (port-per-channel).
        const int freqPortOffset  = numChans;      // port1: gain control

        for (int ch = 0; ch < numChans; ++ch)
        {
            float* out = outputs[ch];
            if (out == nullptr)
                continue;

            const float* in       = (inputs != nullptr) ? inputs[ch] : nullptr;
            const float* freqCtrl = (inputs != nullptr) ? inputs[freqPortOffset + ch] : nullptr;

            if (in == nullptr || freqCtrl == nullptr) // Missing input channels
            {
                std::memset (out, 0, (size_t) numSamples * sizeof (float));
                continue;
            }

            float scalingFactor = pow(2, float(shift) / 12.0);

            // pitchShift2 works better than pitchShift, but both use different methods of shifting
            pitchShift2(numSamples, in, out, freqCtrl[0], scalingFactor);
        }
    }

private:
    int numChans = 1;
    double sampleRate = 48000.0;
    float shift = 0.0f;
    std::vector<puke::dsp::DelayRing> rings;
};
} // namespace PUKE_CUSTOM_NAMESPACE
