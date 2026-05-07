# PukePracticum
3 custom Puke nodes.

To use these nodes and build off of them on your local device, you can just copy the entire file into your custom nodes folder into a local Puke project. These nodes were created for and used in 3 different Autotune implementations in a HU 3910 Pracitcum in Music Technology.

For Autotune with custom Autotune node (AutotuneDSPNode.h and AutotuneNode.cpp):
In puke_node_catalog.h
Add one line next to fx_harmonizer. The pattern is identical:
cpp  X(user, "fx_harmonizer", registerHarmonizerNode, makeNode<HarmonizerNode>, false, false, 1)      \  // <---- already existed in the file
  X(user, "fx_autotune", registerAutotuneNode, makeNode<AutotuneNode>, false, false, 0)            \    // <--------add this

src/nodes/dsp/AllDSPNodes.h
Add one include after HarmonizerDSPNode.h:
cpp#include "HarmonizerDSPNode.h" // <---- already existed in the file
#include "AutotuneDSPNode.h" // <--------add this

## Custom Autotune Node
This is one of the 3 ways to custome a C++ node we talked in report. For this way, we will create an Autotune interface in the Puke app. These are the two files related:

- `AutotuneDSPNode.h`
- `AutotuneNode.cpp`

Follow the steps below.

### 1. Update `puke_node_catalog.h`

Add one line next to `fx_harmonizer`. The pattern is identical to the existing harmonizer registration:

```cpp
X(user, "fx_harmonizer", registerHarmonizerNode, makeNode<HarmonizerNode>, false, false, 1)      \  // <----- already exists
X(user, "fx_autotune", registerAutotuneNode, makeNode<AutotuneNode>, false, false, 0)            \  // <----- add this
```

### 2. Update `src/nodes/dsp/AllDSPNodes.h`

Add one include after `HarmonizerDSPNode.h`:

```cpp
#include "HarmonizerDSPNode.h" // <----- already exists
#include "AutotuneDSPNode.h"  // <----- add this
```
  
