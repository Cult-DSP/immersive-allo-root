# Agent Instructions: Build `immersive-allo-root` From `allo-i-players` Using Embedded Spatial Root

## 1. Mission

Create a new repository called `immersive-allo-root` based on the existing `allo-i-players` project, but replace the old static `adm-allo-player` audio layer with an embedded Spatial Root `EngineSession`.

The new app should remain an AlloLib immersive media application. AlloLib should continue to own the distributed graphics app, shader rendering, window lifecycle, keyboard interaction, and audio callback. Spatial Root should become the internal spatial audio engine used by the app.

The goal is not to launch the Spatial Root GUI. The goal is to embed Spatial Root directly into this AlloLib app and expose a small set of on-screen controls for runtime parameters.

The intended result is an AES AVARIG demo showing that Spatial Root can operate as embeddable infrastructure inside an existing immersive media application.

The source/reference repo is:

```text
https://github.com/lucianpar/allo-i-players
```

The new repo should be:

```text
https://github.com/lucianpar/immersive-allo-root
```

If the new repo already exists, modify it in place. If it does not exist yet, create it from the useful parts of `allo-i-players` while removing the old static player dependency.

---

## 2. High-Level Target

Current conceptual model in `allo-i-players`:

```text
AlloLib DistributedApp
    |
    |-- shader sphere / visual layer
    |-- adm-allo-player static multichannel playback
            |
            |-- pre-decoded 54.1 / 56 channel file
            |-- fixed AlloSphere channel mapping
            |-- direct output to hardware channels
```

Target conceptual model in `immersive-allo-root`:

```text
AlloLib DistributedApp
    |
    |-- shader sphere / visual layer
    |-- on-screen Spatial Root controls
    |-- embedded Spatial Root EngineSession
            |
            |-- load LUSID package or compatible scene input
            |-- load AlloSphere layout JSON
            |-- render through Spatial Root
            |-- output through the AlloLib-owned audio callback
```

The updated repo should demonstrate:

```text
Spatial Root standalone mode:
    Spatial Root owns GUI, backend, audio device, and playback lifecycle.

Spatial Root embedding mode:
    Host app owns window, graphics, UI, lifecycle, and audio callback.
    Spatial Root owns scene loading, layout interpretation, runtime parameters, and spatial rendering.
```

For `immersive-allo-root`, use embedding mode.

---

## 3. Non-Negotiable Constraints

### 3.1 Remove `adm-allo-player` Completely

Do not keep `adm-allo-player` as an active dependency, fallback player, compatibility layer, hidden submodule, or optional legacy path.

Remove from the new repo:

```text
adm-allo-player/
.gitmodules entry for adm-allo-player
#include "adm-allo-player/mainplayer.hpp"
adm_player adm_player_instance
adm_player_instance.onInit(...)
adm_player_instance.onCreate(...)
adm_player_instance.onDraw(...)
adm_player_instance.onKeyDown(...)
adm_player_instance.onSound(...)
```

If code from `allo-i-players` documents itself as a static pre-decoded ADM multichannel player, update that language.

The new project identity should be:

```text
immersive-allo-root demonstrates Spatial Root embedded in an AlloLib distributed immersive media application.
```

### 3.2 Do Not Launch the Spatial Root GUI

Do not open, embed, wrap, shell out to, or depend on the Spatial Root standalone GUI.

Forbidden patterns:

```text
system("Spatial Root.app")
subprocess launch of Spatial Root GUI
GUI-to-GUI communication bridge
Spatial Root GUI as a required runtime process
```

The AlloLib app itself should provide a minimal on-screen control panel for Spatial Root parameters.

### 3.3 Avoid Two Independent Audio Device Owners

AlloLib should own the audio device and audio callback for this app.

Avoid this:

```text
AlloLib opens CoreAudio / JACK / WASAPI device
Spatial Root separately opens CoreAudio / JACK / WASAPI device
```

This creates device selection ambiguity and is not a true embedding demo.

The preferred model is:

```cpp
void MyApp::onSound(al::AudioIOData& io) {
    spatialRootHost.renderAudio(io);
}
```

Spatial Root should render into the host-provided callback buffer.

### 3.4 Keep the Visual Host Intact

Do not rewrite the shader app unless required for integration cleanup.

Keep the existing AlloLib distributed application structure, shader sphere, timing model, shader selection behavior, and visual rendering pipeline as much as possible.

The purpose is to swap the audio engine, not replace the whole app.

### 3.5 Prioritize Clarity Over Abstraction

This is a demo app and integration reference. Prefer a clear, small wrapper over a generalized framework.

Do not over-engineer a plugin architecture, dependency manager, or full application framework unless it is required for Spatial Root integration.

---

## 4. Repository Strategy

### 4.1 Source Repo

Use `allo-i-players` as the source/reference repository.

Preserve useful parts:

```text
AlloLib DistributedApp structure
shader sphere rendering
shader selection behavior
Common state / distributed state logic
basic keyboard transport model
AlloSphere-oriented application framing
```

Remove obsolete parts:

```text
adm-allo-player submodule
static multichannel player controls
ADM player file picker assumptions
fixed pre-decoded 54.1 playback language
old README framing around pre-decoded ADM files
```

### 4.2 New Repo Identity

The new repository is `immersive-allo-root`.

Recommended README one-liner:

```text
immersive-allo-root is an AlloLib immersive media host that embeds Spatial Root as its spatial audio runtime, combining distributed shader rendering with layout-aware LUSID/ADM scene playback.
```

Recommended project description:

```text
An embedded Spatial Root + AlloLib demo for immersive media applications.
```

### 4.3 Suggested Directory Layout

Prefer a small, clear structure:

```text
immersive-allo-root/
    CMakeLists.txt
    README.md
    AGENTS.md
    docs/
        SPATIAL_ROOT_EMBEDDING.md
        CONTROLS.md
        DEVELOPMENT.md
    src/
        main.cpp
        ImmersiveAlloRootApp.hpp
        ImmersiveAlloRootApp.cpp
        SpatialRootAlloHost.hpp
        SpatialRootAlloHost.cpp
        OverlayControls.hpp
        OverlayControls.cpp
    assets/
        shaders/
        layouts/
        scenes/
    external/
        spatial-root/        optional submodule or externally supplied dependency
        allolib/             if this repo vendors AlloLib
```

If the existing repo is currently a single-file app, it is acceptable to keep the first pass simple:

```text
immersivePlayer.cpp
SpatialRootAlloHost.hpp
SpatialRootAlloHost.cpp
```

However, a `src/` split is preferred for long-term clarity.

---

## 5. Expected Architecture

Introduce a small host wrapper around Spatial Root. Suggested names:

```text
SpatialRootAlloHost
SpatialRootHost
SpatialRootEmbeddedHost
```

Preferred files:

```text
src/SpatialRootAlloHost.hpp
src/SpatialRootAlloHost.cpp
```

Suggested structure:

```cpp
class SpatialRootAlloHost {
public:
    bool setup(const SpatialRootHostConfig& config);
    void update(double dt);
    void drawControls(al::Graphics& g);
    bool keyDown(const al::Keyboard& k);
    void renderAudio(al::AudioIOData& io);

    void setPaused(bool paused);
    void setMasterGainDb(float db);
    void setDbapFocus(float focus);
    void setSpeakerMixDb(float db);
    void setSubMixDb(float db);
    void setElevationMode(int mode);
    void cycleElevationMode();

    bool isReady() const;
    bool isPaused() const;
    const std::string& lastError() const;

private:
    spatialroot::EngineSession session;
    spatialroot::RuntimeParams runtime;
    bool ready = false;
    bool paused = true;
    std::string error;
};
```

Adjust names to match the actual Spatial Root API. Do not invent types if the real API already provides equivalent names.

The main app should be simple:

```cpp
class ImmersiveAlloRootApp : public al::DistributedAppWithState<CommonState> {
public:
    SpatialRootAlloHost spatialRoot;

    void onCreate() override {
        // Existing shader setup remains.
        // Spatial Root setup is initialized here or in onInit depending on current app structure.
    }

    void onAnimate(double dt) override {
        // Existing visual timing remains.
        // Runtime parameters are pushed to Spatial Root here.
        spatialRoot.update(dt);
    }

    void onDraw(al::Graphics& g) override {
        // Existing shader draw remains.
        // Draw Spatial Root overlay controls on top.
        spatialRoot.drawControls(g);
    }

    bool onKeyDown(const al::Keyboard& k) override {
        // Existing shader selection remains.
        // Spatial Root controls are handled here.
        return spatialRoot.keyDown(k);
    }

    void onSound(al::AudioIOData& io) override {
        spatialRoot.renderAudio(io);
    }
};
```

---

## 6. Spatial Root Integration Model

### 6.1 Preferred Lifecycle

Use the staged Spatial Root `EngineSession` lifecycle as the integration model.

Expected conceptual sequence:

```cpp
spatialroot::EngineOptions engineOptions;
engineOptions.sampleRate = 48000;
engineOptions.blockSize = 512;
engineOptions.hostOwnedAudio = true; // Use the actual API name if different.

session.configureEngine(engineOptions);

spatialroot::SceneInput sceneInput;
sceneInput.path = "assets/scenes/example.scene.lusid.json"; // Or package root.
session.loadScene(sceneInput);

spatialroot::LayoutInput layoutInput;
layoutInput.path = "assets/layouts/allosphere.layout.json";
session.applyLayout(layoutInput);

spatialroot::RuntimeParams runtime = spatialroot::RuntimeParams::defaultParams();
runtime.masterGainDb = 0.0f;
runtime.dbapFocus = 1.0f;
runtime.subMixDb = 0.0f;
session.configureRuntime(runtime);

session.prepareForExternalAudioCallback(); // Use actual API name if available.
```

Do not assume the exact method names above exist. Inspect the Spatial Root API and use the real names.

If Spatial Root currently lacks an external callback mode, document that as a blocking integration issue and implement the smallest appropriate addition in Spatial Root rather than working around it by launching the GUI or opening a second device.

### 6.2 Audio Callback Requirement

The AlloLib app should call Spatial Root from `onSound`:

```cpp
void ImmersiveAlloRootApp::onSound(al::AudioIOData& io) {
    spatialRoot.renderAudio(io);
}
```

Inside `SpatialRootAlloHost::renderAudio`, Spatial Root should render the next block of audio into the host callback buffers.

Conceptual behavior:

```cpp
void SpatialRootAlloHost::renderAudio(al::AudioIOData& io) {
    if (!ready || paused) {
        while (io()) {
            for (int c = 0; c < io.channelsOut(); ++c) {
                io.out(c) = 0.0f;
            }
        }
        return;
    }

    session.renderNextBlock(io); // Use actual API or adapter.
}
```

The real implementation may need an intermediate non-interleaved or interleaved buffer depending on the Spatial Root render API.

### 6.3 Buffer Conversion

If Spatial Root renders into an internal float buffer, write an adapter with explicit channel handling.

Example conceptual flow:

```text
AlloLib AudioIOData
    -> ask Spatial Root to render N frames x M channels
    -> copy rendered channels to io.out(channel)
    -> zero any unused device output channels
    -> guard against channel-count mismatch
```

Do not assume device channels are contiguous unless the layout says so. Spatial Root should handle internal render bus to output bus mapping where possible.

### 6.4 Realtime Safety

The audio callback must not perform:

```text
disk I/O
scene loading
layout parsing
memory allocation in normal operation
logging every sample/block
mutex blocking
GUI drawing
JSON parsing
CULT transcoding
```

All scene loading, layout loading, and stream preparation must happen before playback or on a non-audio thread with proper synchronization.

If a runtime parameter changes, apply it using the intended Spatial Root realtime-safe mechanism.

---

## 7. Runtime Parameters and Controls

The app should expose a small on-screen control panel for Spatial Root.

Minimum controls:

```text
Transport: Playing / Paused
Master Gain: -60 dB to +12 dB, default 0 dB
DBAP Focus: 0.1 to 5.0, default 1.0
Speaker Mix: -60 dB to +12 dB, default 0 dB if available
Sub Mix: -60 dB to +12 dB or equivalent, default 0 dB if available
Elevation Mode: cycle supported Spatial Root modes
Status: Ready / Loading / Error
Scene path
Layout path
```

Use the actual Spatial Root parameter names and ranges if they differ from the above. If the current Spatial Root API already defines defaults and clamps, use those instead of duplicating policy incorrectly.

### 7.1 AlloLib Parameter Bridge

Where possible, back controls with AlloLib parameters so they can be synchronized through the distributed app model.

Suggested parameters:

```cpp
al::Parameter masterGainDb{"masterGainDb", "", 0.0f, -60.0f, 12.0f};
al::Parameter dbapFocus{"dbapFocus", "", 1.0f, 0.1f, 5.0f};
al::Parameter speakerMixDb{"speakerMixDb", "", 0.0f, -60.0f, 12.0f};
al::Parameter subMixDb{"subMixDb", "", 0.0f, -60.0f, 12.0f};
al::ParameterInt elevationMode{"elevationMode", "", 0, 0, 2};
al::ParameterBool running{"running", "", false};
al::ParameterBool showOverlay{"showOverlay", "", true};
```

If the current app already has equivalent parameters, reuse them.

### 7.2 Parameter Update Path

The main app should push parameter values to Spatial Root outside the audio callback, likely in `onAnimate` or a safe update/tick method.

Conceptual pattern:

```cpp
void ImmersiveAlloRootApp::onAnimate(double dt) {
    spatialRoot.setPaused(!running.get());
    spatialRoot.setMasterGainDb(masterGainDb.get());
    spatialRoot.setDbapFocus(dbapFocus.get());
    spatialRoot.setSpeakerMixDb(speakerMixDb.get());
    spatialRoot.setSubMixDb(subMixDb.get());
    spatialRoot.setElevationMode(elevationMode.get());
    spatialRoot.update(dt);
}
```

Avoid changing complex engine state directly from key handlers. Key handlers should modify parameters; the update path should apply those parameters to Spatial Root.

---

## 8. Keyboard Controls

Keep the existing shader controls where possible. Add Spatial Root controls without breaking shader selection.

Suggested controls:

```text
space       toggle play/pause for audio and visuals
1-9         select shader / visual mode, preserve existing behavior
g           master gain down
G           master gain up
f           DBAP focus down
F           DBAP focus up
s           sub mix down
S           sub mix up
m           speaker mix down
M           speaker mix up
e           cycle elevation mode
r           reload current scene and layout safely, if implemented
h           show/hide overlay help
```

If uppercase key detection is awkward in the current AlloLib keyboard API, use alternate keys. Document the final bindings in `docs/CONTROLS.md` and the README.

The first pass does not need a full file browser or scene browser. Fixed config paths are acceptable for the initial demo.

---

## 9. On-Screen Overlay

The overlay should be minimal and readable. It should not attempt to recreate the Spatial Root GUI.

Suggested content:

```text
IMMERSIVE ALLO ROOT
Spatial Root: Ready
Transport: Playing
Scene: assets/scenes/example.scene.lusid.json
Layout: assets/layouts/allosphere.layout.json
Master Gain: 0.0 dB
DBAP Focus: 1.00
Speaker Mix: 0.0 dB
Sub Mix: 0.0 dB
Elevation: Rescale Atmos Up

Keys: Space play/pause | F/f focus | G/g gain | E elevation | H hide
```

Use an existing AlloLib text rendering path if available. If text rendering is not already set up, use the simplest available overlay mechanism. Do not spend excessive effort on UI polish before the audio path is working.

If text rendering becomes a distraction, temporarily print parameter state to console and create the overlay in a second pass.

---

## 10. CMake and Dependency Strategy

### 10.1 Remove Old Dependency

Remove `adm-allo-player` from:

```text
.gitmodules
CMakeLists.txt
include paths
submodule update instructions
README dependency list
```

Run:

```bash
git submodule deinit -f adm-allo-player || true
git rm -f adm-allo-player || true
rm -rf .git/modules/adm-allo-player
```

Only run destructive commands after confirming repository state. If this is being done by an agent, inspect before removing.

### 10.2 Add Spatial Root

Preferred options, in order:

1. Use Spatial Root as a git submodule on the intended branch, likely `devel`.
2. Use a local path override for development.
3. Use an installed package only if Spatial Root already supports that cleanly.

Suggested submodule path:

```text
external/spatial-root
```

Example:

```bash
git submodule add -b devel https://github.com/Cult-DSP/SpatialRoot.git external/spatial-root
```

Use the actual repository URL and branch name used by the project. Do not guess if the URL differs.

### 10.3 CMake Shape

The CMake build should clearly include:

```text
AlloLib
Spatial Root engine/library target
immersive-allo-root app executable
```

Avoid linking the Spatial Root GUI target.

Conceptual CMake:

```cmake
cmake_minimum_required(VERSION 3.20)
project(immersive-allo-root LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/spatial-root)
# Add or locate AlloLib according to current repo convention.

add_executable(immersive-allo-root
    src/main.cpp
    src/ImmersiveAlloRootApp.cpp
    src/SpatialRootAlloHost.cpp
    src/OverlayControls.cpp
)

target_include_directories(immersive-allo-root PRIVATE
    src
)

target_link_libraries(immersive-allo-root PRIVATE
    spatialroot_realtime       # Use actual target name.
    al                         # Use actual AlloLib target name.
)
```

Use the real target names from Spatial Root and AlloLib. Do not invent or hardcode names without checking.

### 10.4 Build Flags

Spatial Root may provide multiple targets such as engine, offline renderer, GUI, dev tools, or transcoder. For this app, enable only what is required for embedded realtime playback.

Preferred:

```text
ENGINE / realtime playback: ON
GUI: OFF
OFFLINE: OFF unless needed
CULT: OFF unless the app performs transcoding directly
DEVTOOLS: OFF
```

If the build system does not currently support this separation, document the issue and make the smallest build change required.

---

## 11. Scene and Layout Assets

### 11.1 Scene Input

Initial version may use a fixed scene path:

```text
assets/scenes/example.scene.lusid.json
```

or a package root:

```text
assets/scenes/example_lusid_package/
    scene.lusid.json
    audio/
    metadata/
```

Use whatever Spatial Root currently expects.

### 11.2 Layout Input

Initial version should use an AlloSphere-oriented layout JSON:

```text
assets/layouts/allosphere.layout.json
```

This layout should contain the device/output channel mapping expected by Spatial Root.

Do not duplicate speaker mapping logic in `immersive-allo-root` if Spatial Root already owns layout parsing and output mapping.

### 11.3 Asset Policy

Do not commit large copyrighted audio files.

Acceptable assets:

```text
small synthetic LUSID test scene
short generated mono stems
small placeholder scene package
layout JSON
README instructions for adding local scene packages
```

If the demo needs large local media, document the expected local path and add it to `.gitignore`.

---

## 12. Handling Missing Spatial Root Embedding API

This task may reveal that Spatial Root currently assumes it owns the audio backend. If so, do not work around the problem with an external GUI process or a second audio device.

Instead, report the issue clearly and implement the smallest useful Spatial Root-side addition.

The needed feature is:

```text
External callback / host-owned audio mode
```

Minimum required behavior:

```text
configure engine without opening a device
load scene
apply layout
configure runtime params
prepare streaming/rendering state
render N frames into a caller-provided output buffer
pause/resume safely
shutdown without owning host audio device
```

Possible Spatial Root API addition:

```cpp
class EngineSession {
public:
    bool configureEngine(const EngineOptions& options);
    bool loadScene(const SceneInput& input);
    bool applyLayout(const LayoutInput& input);
    bool configureRuntime(const RuntimeParams& params);

    bool prepareExternalRender(int sampleRate, int blockSize, int outputChannels);
    bool renderExternal(float** outputs, int numChannels, int numFrames);
    bool renderExternalInterleaved(float* output, int numChannels, int numFrames);

    void setPaused(bool paused);
    void update();
    void shutdown();
};
```

Names do not matter. The behavior matters.

Do not add this as a hack specific to AlloLib. It should be useful for Unreal, TouchDesigner, Max/MSP, and other embedding hosts later.

---

## 13. Error Handling

The app should fail clearly when setup is incomplete.

Handle at least:

```text
Spatial Root dependency missing or not built
scene file missing
layout file missing
scene load failure
layout parse failure
sample rate mismatch
output channel count mismatch
render callback called before ready
runtime parameter outside valid range
```

The app should not crash on missing scene/layout. It should show an error in the overlay and output silence.

Suggested behavior:

```cpp
if (!spatialRoot.setup(config)) {
    std::cerr << "Spatial Root setup failed: " << spatialRoot.lastError() << std::endl;
    // Continue app with visual-only mode and silent audio.
}
```

Overlay should show:

```text
Spatial Root: Error
<last error message>
```

---

## 14. Testing Checklist

### 14.1 Repo Cleanup

Verify:

```text
No adm-allo-player submodule remains.
No adm_player symbols remain.
No includes from adm-allo-player remain.
README no longer describes the app as a static 54.1 ADM player.
CMake does not reference adm-allo-player.
```

Suggested commands:

```bash
grep -R "adm-allo-player\|adm_player\|mainplayer" -n . \
  --exclude-dir=.git \
  --exclude-dir=build
```

The only allowed matches should be in migration notes if such notes are intentionally kept.

### 14.2 Build

Verify clean configure and build:

```bash
cmake -S . -B build
cmake --build build -j
```

If Spatial Root requires explicit options, document the exact configure command:

```bash
cmake -S . -B build \
  -DSPATIALROOT_BUILD_ENGINE=ON \
  -DSPATIALROOT_BUILD_GUI=OFF
```

Use real option names.

### 14.3 Runtime Smoke Test

Verify:

```text
app launches
shader sphere still renders
keyboard shader switching still works
overlay appears
scene and layout load or a clear error appears
audio callback runs without crash
paused state outputs silence
playing state outputs Spatial Root render output
master gain changes level
DBAP focus changes localization/rendering behavior
sub/speaker mix controls do not crash
app exits cleanly
```

### 14.4 Realtime Safety Smoke Test

Check for obvious mistakes:

```text
No scene loading inside onSound
No JSON parsing inside onSound
No logging every audio block
No allocation-heavy work inside onSound
No blocking locks inside onSound
```

### 14.5 Channel Count Test

Test with expected AlloSphere output count and with smaller local output count if possible.

The app should not crash if the local device has fewer channels than the AlloSphere layout. It may warn and render only available channels or refuse to start audio clearly.

Do not write past available output channels.

---

## 15. Documentation Updates

### 15.1 README

The README should include:

```text
What immersive-allo-root is
Why it exists
How it differs from allo-i-players
How Spatial Root is embedded
Build instructions
Runtime asset expectations
Keyboard controls
Known limitations
```

Suggested README intro:

```markdown
# immersive-allo-root

`immersive-allo-root` is an AlloLib immersive media application that embeds Spatial Root as its spatial audio runtime. It combines distributed shader rendering with layout-aware spatial audio playback, demonstrating how Spatial Root can operate inside an existing host application rather than only as a standalone GUI.
```

Suggested distinction:

```markdown
This project is derived from the earlier `allo-i-players` static multichannel player, but it removes `adm-allo-player` entirely. Audio playback is now handled by an embedded Spatial Root `EngineSession` using scene and layout data rather than a fixed pre-decoded channel file.
```

### 15.2 `docs/SPATIAL_ROOT_EMBEDDING.md`

Document:

```text
host-owned audio callback model
EngineSession lifecycle
scene/layout loading
runtime parameter bridge
why the Spatial Root GUI is not used
how this informs other embedding targets
```

### 15.3 `docs/CONTROLS.md`

Document final keyboard controls and overlay behavior.

### 15.4 `docs/DEVELOPMENT.md`

Document:

```text
submodule setup
CMake options
asset paths
local testing procedure
known limitations
future work
```

---

## 16. Future Work, Not First Pass

Do not block the first implementation on:

```text
full file browser
full Spatial Root GUI parity
CULT transcoding from inside this app
ADM import/export inside this app
scene editing
advanced visualization of object trajectories
networked LUSID streaming
Unreal integration
TouchDesigner integration
Max/MSP plugin integration
packaged installer
```

Those are useful future directions, but the first pass should prove the embedded Spatial Root audio path.

---

## 17. Paper / Demo Framing

This repo should support the AES AVARIG paper distinction from the Linux Audio Conference paper.

Linux paper framing:

```text
Overview of the CULT DSP / Spatial Root / LUSID toolchain.
```

AES demo framing:

```text
Spatial Root as embeddable infrastructure for immersive media applications.
```

Useful sentence:

```text
immersive-allo-root demonstrates that Spatial Root can be embedded into an existing AlloLib distributed graphics application, allowing a host environment to retain control over visuals, lifecycle, and the audio device while delegating scene interpretation, layout-aware rendering, and runtime spatial parameters to Spatial Root.
```

Another useful sentence:

```text
Unlike the earlier static AlloSphere player, which played pre-decoded multichannel files through fixed channel mappings, immersive-allo-root uses Spatial Root to render scene data against a target loudspeaker layout at runtime.
```

---

## 18. Final Deliverables

The implementation pass should deliver:

```text
1. New or updated `immersive-allo-root` repository.
2. `adm-allo-player` fully removed.
3. Spatial Root dependency added without using the Spatial Root GUI.
4. Embedded `EngineSession` wrapper added.
5. AlloLib `onSound` routes to Spatial Root render path.
6. Minimal on-screen controls for runtime parameters.
7. Keyboard controls documented.
8. README rewritten for the new repo identity.
9. Build instructions verified.
10. Clear notes on any Spatial Root API gap discovered during embedding.
```

The most important success criterion:

```text
The app is no longer a static multichannel file player. It is an AlloLib immersive host with Spatial Root embedded as the audio engine.
```

---

## 19. Implementation Style

Keep changes concrete and reviewable.

Prefer:

```text
small wrapper class
clear lifecycle calls
explicit error messages
minimal overlay
documented asset paths
simple keyboard controls
```

Avoid:

```text
large speculative refactors
new GUI framework
launching the Spatial Root GUI
duplicating Spatial Root layout logic
keeping adm-allo-player as fallback
hiding errors behind silent failure
```

If blocked, report the exact blocker and the smallest next patch required. Do not replace the architecture with a workaround that violates the constraints above.
