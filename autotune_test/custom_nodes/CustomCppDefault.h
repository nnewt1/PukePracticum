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
static constexpr const char* kNodeLabel         = "Custom C++";
static constexpr const char* kNodeDescription   = "Delay with audio-rate delay, feedback, and master gain.";
static constexpr const char* kNodeCategory      = "Custom";
static constexpr const char* kNodeSubcategory   = "DSP";

// Params define UI controls and optional param ports.
// To add a new UI control, append a new entry here and handle it in setParam().
static const std::array<PukeCustomParamDesc, 3> kUserParams = {{
    { "gain", "Gain", PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE,
      0.0, 2.0, 1.0, nullptr, nullptr, 0,
      PUKE_CUSTOM_PARAM_USE_SLIDER | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "delay_samples", "Delay (samples)", PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE,
      0.0, (double) PUKE_SIMPLE_DELAY_MAX_DELAY_SAMPLES, 0.0, nullptr, nullptr, 0,
      PUKE_CUSTOM_PARAM_USE_SLIDER | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "feedback", "Feedback", PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE,
      0.0, 0.99, 0.0, nullptr, nullptr, 0,
      PUKE_CUSTOM_PARAM_USE_SLIDER | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT }
}};

// Signal ports are audio-rate and show up in inputs/outputs arrays.
// To add a port, append here and update the offsets in process().
// Port indices are packed by port (mono):
//   inputs[0] -> default In
//   inputs[1] -> first extra port ("gain")
//   inputs[2] -> second extra port ("delay_samples")
//   inputs[3] -> third extra port ("feedback")
static const std::array<PukeCustomPortDesc, 3> kUserPorts = {{
    // Extra signal ports after the default In/Out.
    { "gain", "Gain In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "delay_samples", "Delay In (samples)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "feedback", "Feedback In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN }
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

// DSP processor for this node.
// Write your custom audio code inside process().
/// Type for Processor.
/// DSP processor for this node.
/// Write your custom audio code inside process().
class Processor
{
public:
    void prepare (double /*sr*/, int /*blockSize*/, int nChans)
    {
        numChans = std::max (1, nChans);
        const int maxDelay = std::max (1, (int) PUKE_SIMPLE_DELAY_MAX_DELAY_SAMPLES);
        rings.resize ((size_t) numChans);
        for (int ch = 0; ch < numChans; ++ch)
            rings[(size_t) ch].prepare (maxDelay);
    }

   #if defined(PUKE_CUSTOM_STATIC)
    void setParam (const char* id, float value)
    {
        if (id == nullptr)
            return;

        if (std::strcmp (id, "gain") == 0)
            gain = value;
        else if (std::strcmp (id, "delay_samples") == 0)
            delaySamples = value;
        else if (std::strcmp (id, "feedback") == 0)
            feedback = value;
    }
   #else
    void setParam (const juce::String& id, float value)
    {
        if (id == "gain")
            gain = value;
        else if (id == "delay_samples")
            delaySamples = value;
        else if (id == "feedback")
            feedback = value;
    }
   #endif

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
        const int gainPortOffset  = numChans;      // port1: gain control
        const int delayPortOffset = numChans * 2;  // port2: delay control
        const int fbPortOffset    = numChans * 3;  // port3: feedback control

        for (int ch = 0; ch < numChans; ++ch)
        {
            float* out = outputs[ch];
            if (out == nullptr)
                continue;

            const float* in       = (inputs != nullptr) ? inputs[ch] : nullptr;
            const float* gainCtrl = (inputs != nullptr) ? inputs[gainPortOffset + ch] : nullptr;
            const float* delayCtl = (inputs != nullptr) ? inputs[delayPortOffset + ch] : nullptr;
            const float* fbCtrl   = (inputs != nullptr) ? inputs[fbPortOffset + ch] : nullptr;

            puke::dsp::DelayRing& ring = rings[(size_t) ch];
            float* bufData = ring.data();
            const int size = ring.size();

            if (bufData == nullptr || size <= 0)
            {
                if (in != nullptr)
                    std::memcpy (out, in, (size_t) numSamples * sizeof (float));
                else
                    std::memset (out, 0, (size_t) numSamples * sizeof (float));
                continue;
            }

            int w = ring.writePos;

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = (in != nullptr) ? in[i] : 0.0f;

                const float g  = (gainCtrl != nullptr) ? gainCtrl[i] : gain;
                const float ds = (delayCtl != nullptr) ? delayCtl[i] : delaySamples;
                const float fb = (fbCtrl != nullptr) ? fbCtrl[i] : feedback;

                int delaySamps = (int) std::lround (ds);
                if (delaySamps < 0)
                    delaySamps = 0;
                if (delaySamps > size - 1)
                    delaySamps = size - 1;

                float fbClamped = fb;
                if (fbClamped < 0.0f)  fbClamped = 0.0f;
                if (fbClamped > 0.99f) fbClamped = 0.99f;

                int readPos = w - delaySamps;
                if (readPos < 0)
                    readPos += size;

                const float delayed = bufData[(size_t) readPos];
                out[i] = delayed * g;

                // Write input + feedback back into the delay line.
                bufData[(size_t) w] = dry + (delayed * fbClamped);

                if (++w >= size)
                    w = 0;
            }

            ring.writePos = w;
        }
    }

private:
    int numChans = 1;
    float gain = 1.0f;
    float delaySamples = 0.0f;
    float feedback = 0.0f;
    std::vector<puke::dsp::DelayRing> rings;
};
} // namespace PUKE_CUSTOM_NAMESPACE
