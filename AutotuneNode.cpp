// file source: src/nodes/defs/AutotuneNode.cpp
/// @brief Node definition and registration for Autotune.

#include "nodes/NodeDefHelpers.h"
#include "registry/NodeDefinition.h"
#include "registry/NodeRegistry.h"

#include "nodes/dsp/AutotuneDSPNode.h"

namespace {

/// Build the node definition for Autotune.
/// AUTOTUNE (pitch correction via MPM detection + Harmonizer shift)
NodeDefinition makeAutotuneDefinition() {
  NodeDefinition def;
  def.typeId = "fx_autotune";
  def.label = "Autotune";
  def.description =
      "Monophonic pitch correction. Detects the fundamental via MPM/NSDF, "
      "quantises to the nearest in-scale note, and shifts pitch using a "
      "windowed delay-head shifter.";
  setParamsUI(def);
  setMenuPath(def, "FX", "Pitch");

  // --- key (0=C, 1=C#, 2=D … 11=B) ----------------------------------------
  {
    NodeParamDescriptor p;
    p.id = "key";
    p.label = "Key";
    p.tooltip = "Root note of the scale (C=0, C#=1, D=2 ... B=11).";
    p.direction = NodeParamDescriptor::Direction::None;
    p.minValue = 0.0;
    p.maxValue = 11.0;
    p.defaultValue = 0.0;
    p.step = 1;
    p.presets.add({"C",  0.0});
    p.presets.add({"C#", 1.0});
    p.presets.add({"D",  2.0});
    p.presets.add({"D#", 3.0});
    p.presets.add({"E",  4.0});
    p.presets.add({"F",  5.0});
    p.presets.add({"F#", 6.0});
    p.presets.add({"G",  7.0});
    p.presets.add({"G#", 8.0});
    p.presets.add({"A",  9.0});
    p.presets.add({"A#", 10.0});
    p.presets.add({"B",  11.0});
    addParam(def, p);
  }

  // --- scale (0=chromatic, 1=major, 2=minor) --------------------------------
  {
    NodeParamDescriptor p;
    p.id = "scale";
    p.label = "Scale";
    p.tooltip =
        "Which notes are legal targets. "
        "Chromatic = all 12, Major = W W H W W W H, Minor = natural minor.";
    p.direction = NodeParamDescriptor::Direction::None;
    p.minValue = 0.0;
    p.maxValue = 2.0;
    p.defaultValue = 1.0;
    p.step = 1;
    p.presets.add({"Chromatic", 0.0});
    p.presets.add({"Major",     1.0});
    p.presets.add({"Minor",     2.0});
    addParam(def, p);
  }

  // --- speed (retune slew, ms) ----------------------------------------------
  {
    NodeParamDescriptor p;
    p.id = "speed";
    p.label = "Speed (ms)";
    p.tooltip =
        "Retune slew time in milliseconds. Low values snap instantly (T-Pain), "
        "higher values give a natural portamento-style correction.";
    p.direction = NodeParamDescriptor::Direction::None;
    p.minValue = 1.0;
    p.maxValue = 250.0;
    p.defaultValue = 25.0;
    p.step = 1;
    p.presets.add({"Instant", 1.0});
    p.presets.add({"Natural", 50.0});
    p.presets.add({"Slow",    150.0});
    addParam(def, p);
  }

  // --- mix ------------------------------------------------------------------
  {
    NodeParamDescriptor p;
    p.id = "mix";
    p.label = "Mix";
    p.tooltip = "Wet/dry blend. 1 = fully corrected, 0 = dry pass-through.";
    p.direction = NodeParamDescriptor::Direction::None;
    p.minValue = 0.0;
    p.maxValue = 1.0;
    p.defaultValue = 1.0;
    p.step = 0.01;
    addParam(def, p);
  }

  // --- window (pitch detector analysis window, exposed as power-user param) -
  {
    NodeParamDescriptor p;
    p.id = "window";
    p.label = "Window (samples)";
    p.tooltip =
        "Pitch detection analysis window length. Larger = steadier but slower "
        "to track fast pitch changes. Matches the Pitch Detect node setting.";
    p.direction = NodeParamDescriptor::Direction::None;
    p.minValue = 64.0;
    p.maxValue = 8192.0;
    p.defaultValue = 1024.0;
    p.step = 10;
    p.presets.add({"Stable", 2048.0});
    p.presets.add({"Fast",   512.0});
    addParam(def, p);
  }

  // --- ports ----------------------------------------------------------------
  // Audio in / corrected out.
  addSignal(def, "in",  "In",  Port::Direction::Input);
  addSignal(def, "out", "Out", Port::Direction::Output);

  // Modulation inputs — mirror the params so users can drive them at signal rate.
  addSignal(def, "key",   "Key",        Port::Direction::Input);
  addSignal(def, "scale", "Scale",      Port::Direction::Input);
  addSignal(def, "speed", "Speed (ms)", Port::Direction::Input);
  addSignal(def, "mix",   "Mix",        Port::Direction::Input);

  // Harmonizer also writes dry_out and three voice outputs. Declare them so
  // the engine allocates the required output buffer slots (5 * nChans total).
  // Users who only care about the corrected signal wire "out" and ignore these.
  addSignal(def, "dry_out",    "Dry Out",     Port::Direction::Output);
  addSignal(def, "voice1_out", "Shifted Out", Port::Direction::Output);
  addSignal(def, "voice2_out", "Voice 2 Out", Port::Direction::Output);
  addSignal(def, "voice3_out", "Voice 3 Out", Port::Direction::Output);

  // --- tooltips -------------------------------------------------------------
  configurePort(def, "in",         "Monophonic audio input to be pitch-corrected.");
  configurePort(def, "out",        "Pitch-corrected output with wet/dry mix applied.");
  configurePort(def, "key",        "Root note override at signal rate (0-11).");
  configurePort(def, "scale",      "Scale override at signal rate (0=chromatic, 1=major, 2=minor).");
  configurePort(def, "speed",      "Retune slew override at signal rate (ms).");
  configurePort(def, "mix",        "Wet/dry blend override at signal rate (0-1).");
  configurePort(def, "dry_out",    "Unprocessed dry signal (pre mix).");
  configurePort(def, "voice1_out", "Raw pitch-shifted signal before wet/dry blend.");
  configurePort(def, "voice2_out", "Unused (Harmonizer voice 2, always silent).");
  configurePort(def, "voice3_out", "Unused (Harmonizer voice 3, always silent).");

  // --- DSP factory ----------------------------------------------------------
  def.createDSPNode = []() -> std::unique_ptr<DSPNode> {
    return std::make_unique<AutotuneDSPNode>();
  };

  return def;
}

} // namespace

namespace NodeDefs {
/// Register the Autotune node definition.
bool registerAutotuneNode() {
  return NodeRegistry::registerDefinition([] { return makeAutotuneDefinition(); });
}
} // namespace NodeDefs
