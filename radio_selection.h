#pragma once
// PUKE_CUSTOM_TEMPLATE
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "nodes/custom/CustomCppModuleAPI.h"

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
// Custom C++ node template for a selectable note-value source.
//
// This node exposes 12 selectable choices. Each choice has:
// 1) A hardcoded editable value in the node UI.
// 2) An optional input port that overrides the hardcoded value when connected.
//
// The selected choice is controlled by the "selected" parameter. The output
// emits the currently selected value, either from the UI field or the matching
// connected input port.
//
// Port rules:
// - The engine always provides the default audio In and Out (port 0).
// - Extra signal ports are appended after that and appear in the inputs array.
//
// Safety rule:
// - If you change the number/order of signal ports, re-add the node or reload
//   the patch so the graph recompiles with the new layout.
//
//-------------------------------------------------------------------------
static constexpr const char* kNodeTypeId        = PUKE_CUSTOM_TYPE_ID;
static constexpr const char* kNodeLabel         = "Radio Selection";
static constexpr const char* kNodeDescription   = "Select one of 12 note values, with optional input overrides for each choice.";
static constexpr const char* kNodeCategory      = "Custom";
static constexpr const char* kNodeSubcategory   = "DSP";
static constexpr int kChoiceCount = 12;
static constexpr std::array<const char*, kChoiceCount> kChoiceParamIds = {{
    "note1", "note2", "note3", "note4", "note5", "note6",
    "note7", "note8", "note9", "note10", "note11", "note12"
}};
// Params define UI controls and optional param ports.
// Use a single CHOICE param for selection (guarantees mutual-exclusion in
// hosts that render choice controls), and keep the 12 numeric `noteN` fields
// (text/number inputs) next to each other for editing. Input signal ports
// are commented out below for simplicity.
static constexpr std::array<const char*, kChoiceCount> kChoiceLabels = {{
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
}};

// Params define UI controls and optional param ports.
// Interleave a `selN` button param and `noteN` numeric field so hosts render
// a button beside each numeric input. `sel1` defaults to 1.0 (selected).
static const std::array<PukeCustomParamDesc, kChoiceCount * 2> kUserParams = {{
    { "sel1",  "Select 1",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 1.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note1", "Note 1",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 60.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel2",  "Select 2",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note2", "Note 2",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 61.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel3",  "Select 3",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note3", "Note 3",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 62.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel4",  "Select 4",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note4", "Note 4",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 63.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel5",  "Select 5",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note5", "Note 5",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 64.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel6",  "Select 6",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note6", "Note 6",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 65.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel7",  "Select 7",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note7", "Note 7",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 66.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel8",  "Select 8",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note8", "Note 8",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 67.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel9",  "Select 9",  PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note9", "Note 9",    PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 68.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel10", "Select 10", PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note10","Note 10",   PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 69.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel11", "Select 11", PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note11","Note 11",   PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 70.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },

    { "sel12", "Select 12", PUKE_CUSTOM_PARAM_BOOL,  PUKE_CUSTOM_PARAM_NONE, 0.0, 1.0, 0.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_USE_BUTTON | PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT },
    { "note12","Note 12",   PUKE_CUSTOM_PARAM_FLOAT, PUKE_CUSTOM_PARAM_NONE, 0.0, 127.0, 71.0, nullptr, nullptr, 0, PUKE_CUSTOM_PARAM_EXPOSABLE | PUKE_CUSTOM_PARAM_EXPOSED_BY_DEFAULT }
}};

// Signal ports are audio-rate and show up in inputs/outputs arrays.
// Each choice has a matching optional override input port.
// Signal ports are commented out for now to simplify the UI/layout. Re-enable
// when you want the optional per-choice inputs back.
static const std::array<PukeCustomPortDesc, kChoiceCount> kUserPorts = {{
    { "in1", "Note 1 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in2", "Note 2 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in3", "Note 3 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in4", "Note 4 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in5", "Note 5 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in6", "Note 6 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in7", "Note 7 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in8", "Note 8 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in9", "Note 9 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in10", "Note 10 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in11", "Note 11 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN },
    { "in12", "Note 12 In (signal)", PUKE_CUSTOM_PORT_SIGNAL, PUKE_CUSTOM_PORT_IN }
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
class Processor
{
public:
    void prepare (double /*sr*/, int /*blockSize*/, int nChans)
    {
        numChans = std::max (1, nChans);
    }

   #if defined(PUKE_CUSTOM_STATIC)
    void setParam (const char* id, float value)
    {
        if (id == nullptr)
            return;

        // Handle selN toggles (sel1..sel12). When turned on, set selectedChoice
        // and clear the other sel flags so only one remains active.
        char buf[16];
        for (int si = 0; si < kChoiceCount; ++si)
        {
            std::snprintf (buf, sizeof (buf), "sel%d", si + 1);
            if (std::strcmp (id, buf) == 0)
            {
                if (value > 0.5f)
                {
                    selectedChoice = si;
                    for (int j = 0; j < kChoiceCount; ++j)
                        selFlags[(size_t) j] = 0.0f;
                    selFlags[(size_t) si] = 1.0f;
                }
                else
                {
                    selFlags[(size_t) si] = 0.0f;
                }
                return;
            }
        }

        // Otherwise treat as a noteN value update.
        for (int i = 0; i < kChoiceCount; ++i)
        {
            if (std::strcmp (id, kChoiceParamIds[(size_t) i]) == 0)
            {
                choiceValues[(size_t) i] = value;
                return;
            }
        }
    }
   #else
    void setParam (const juce::String& id, float value)
    {
        // Handle selN toggles
        for (int si = 0; si < kChoiceCount; ++si)
        {
            juce::String selId = "sel" + juce::String (si + 1);
            if (id == selId)
            {
                if (value > 0.5f)
                {
                    selectedChoice = si;
                    for (int j = 0; j < kChoiceCount; ++j)
                        selFlags[(size_t) j] = 0.0f;
                    selFlags[(size_t) si] = 1.0f;
                }
                else
                {
                    selFlags[(size_t) si] = 0.0f;
                }
                return;
            }
        }

        // Note value updates
        for (int i = 0; i < kChoiceCount; ++i)
        {
            if (id == kChoiceParamIds[(size_t) i])
            {
                choiceValues[(size_t) i] = value;
                return;
            }
        }
    }
   #endif

    void process (const float* const* inputs,
                  float* const* outputs,
                  int numSamples)
    {
        if (outputs == nullptr || numSamples <= 0)
            return;

        const int selectedIndex = clampChoiceIndex (selectedChoice);

        for (int ch = 0; ch < numChans; ++ch)
        {
            float* out = outputs[ch];
            if (out == nullptr)
                continue;

            const int portIndex = ((selectedIndex + 1) * numChans) + ch;
            const float* selectedInput = (inputs != nullptr) ? inputs[portIndex] : nullptr;

            if (selectedInput != nullptr)
            {
                for (int i = 0; i < numSamples; ++i)
                    out[i] = selectedInput[i];
                continue;
            }

            for (int i = 0; i < numSamples; ++i)
                out[i] = choiceValues[(size_t) selectedIndex];
        }
    }

private:
    static int clampChoiceIndex (float value)
    {
        int idx = (int) std::lround (value);
        if (idx < 0)
            idx = 0;
        if (idx >= kChoiceCount)
            idx = kChoiceCount - 1;
        return idx;
    }

    int numChans = 1;
    int selectedChoice = 0;
    std::array<float, kChoiceCount> choiceValues {{
        60.0f, 61.0f, 62.0f, 63.0f, 64.0f, 65.0f,
        66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f
    }};
    std::array<float, kChoiceCount> selFlags {{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    }};
};
} // namespace PUKE_CUSTOM_NAMESPACE
