# Implementation Summary: `allo-i-players` → `immersive-allo-root`

## Objective

Transform the existing `allo-i-players` (a static multichannel ADM file player using `adm-allo-player`) into `immersive-allo-root` — an AlloLib immersive media application that embeds Spatial Root as its spatial audio engine.

---

## 1. Removed Dependencies

### `adm-allo-player` (submodule)
- Deinitialized and removed from `.gitmodules`
- All `#include "adm-allo-player/mainplayer.hpp"` references removed
- All `adm_player_instance` calls removed from the app lifecycle (`onInit`, `onCreate`, `onDraw`, `onKeyDown`, `onSound`)

### Root `allolib/` and `al_ext/` (submodules)
- Removed — replaced by Spatial Root's vendored `internal/cult-allolib`
- `al_ext` provided AlloSphere-specific features (`DistributedApp`, `PerProjectionRender`, `sphere::*`) that are not needed for desktop embedding

### `immersivePlayer.cpp` and `shaderDistroRef.cpp`
- Old single-file app sources removed — replaced by `src/main.cpp`

---

## 2. Spatial Root API Additions (submodule changes)

Spatial Root's `EngineSession` previously assumed it owned the audio device. The following changes add a **host-owned audio callback mode**:

### `RealtimeBackend.hpp`
- Added `mNoDeviceMode` constructor parameter and member flag
- `init()`: when `noDeviceMode`, skips AudioIO device enumeration and opening
- `start()`: when `noDeviceMode`, skips `mAudioIO.start()`
- `stop()` / `shutdown()`: when `noDeviceMode`, skip AudioIO operations
- `isRunning()`: when `noDeviceMode`, reports `playing` atomic instead of `mAudioIO.isRunning()`
- Added `processExternal(al::AudioIOData& io)` — public method that delegates to the private `processBlock()` pipeline

### `EngineSession.hpp`
- Added forward declarations for `OutputRemap` and `al::AudioIOData`
- Added `prepareForExternalAudioCallback()` — creates a no-device backend, wires agents, starts loader thread. Call instead of `start()`.
- Added `renderExternal(al::AudioIOData& io)` — calls `mBackend->processExternal(io)` from host audio callback

### `EngineSession.cpp`
- Implemented `prepareForExternalAudioCallback()`: creates `RealtimeBackend` with `noDeviceMode=true`, connects Streaming/Pose/Spatializer agent pointers, starts the disk loader thread
- Implemented `renderExternal()`: delegates to the backend's per-block spatial processing pipeline

---

## 3. New Files

### `src/SpatialRootAlloHost.hpp`
Header-only wrapper class providing a clean interface between AlloLib's `App` and Spatial Root's `EngineSession`:

```
class SpatialRootAlloHost
  bool setup(SpatialRootHostConfig)
  void update(double dt)
  void renderAudio(al::AudioIOData& io)
  void setPaused(bool)
  void setMasterGainDb(float)
  void setDbapFocus(float)
  void setSpeakerMixDb(float)
  void setSubMixDb(float)
  void setElevationMode(int)
  void cycleElevationMode()
```

The host config struct accepts sample rate, block size, scene path, sources/ADM path, and layout path. The wrapper calls the lifecycle in order: `configureEngine` → `loadScene` → `applyLayout` → `configureRuntime` → `prepareForExternalAudioCallback`.

### `src/main.cpp`
New application entry point. `ImmersiveAlloRootApp` extends `al::App` and:
- Loads fragment shaders from `miniShader/demoShaders/`
- Renders a textured icosphere via `ShadedSphere`
- Creates a `SpatialRootAlloHost` instance initialized with configurable scene/layout paths
- Routes audio: `onSound(io)` → `spatialRoot.renderAudio(io)`
- Routes keyboard: Space toggles play/pause, 1-9 selects shaders, G/F/S/M/E control Spatial Root parameters

### `assets/`
- `assets/layouts/allosphere.layout.json` — placeholder speaker layout (replace with real data)
- `assets/scenes/.gitkeep`, `assets/shaders/.gitkeep` — directory structure

---

## 4. CMake Build Changes

### `CMakeLists.txt`
| Before | After |
|--------|-------|
| `project(allo-i-players)` | `project(immersive-allo-root)` |
| Source: `immersivePlayer.cpp` | Source: `src/main.cpp` |
| Links: `al` (root allolib), `al_ext` | Links: `EngineSessionCore`, `al` (via cult-allolib) |
| Submodules: allolib, al_ext, adm-allo-player, miniShader, spatialroot | Submodules: miniShader, spatialroot only |
| `add_subdirectory(allolib)` then `add_subdirectory(al_ext)` then `add_subdirectory(adm-allo-player)` | `add_subdirectory(spatialroot)` only |

Spatial Root build options:
```
SPATIALROOT_BUILD_ENGINE=ON
SPATIALROOT_BUILD_OFFLINE=OFF
SPATIALROOT_BUILD_CULT=OFF
SPATIALROOT_BUILD_GUI=OFF
```

### `al_ControlNav.cpp`
Cult-allolib does not compile `src/io/al_ControlNav.cpp` in any library target, but it is required by `al::App`. Added it directly to the app's source list.

---

## 5. Architecture Summary

```
ImmersiveAlloRootApp (al::App)
├── Shader sphere (ShadedMesh + miniShader)
├── onSound(audio callback)
│   └── SpatialRootAlloHost::renderAudio(io)
│       └── EngineSession::renderExternal(io)
│           └── RealtimeBackend::processExternal(io)
│               └── processBlock(io) — full spatial pipeline:
│                   1. Snapshot runtime atomics
│                   2. Exponential smoothing
│                   3. Pause fade
│                   4. Zero output buffers
│                   5. Pose::computePositions
│                   6. Spatializer::renderBlock (DBAP)
│                   7. Output remap
│                   8. Update frame counter / CPU meter
├── onAnimate → push params to SpatialRootAlloHost
└── onKeyDown → keyboard controls for transport + SR params
```

### Threading
- **Main thread**: `onInit`, `onCreate`, `onAnimate`, `onDraw`, `onKeyDown`, `SpatialRootAlloHost::update`
- **Audio thread** (AlloLib owned): `onSound` → `renderExternal` → `processBlock`
- **Loader thread** (SpatialRoot owned): disk I/O for WAV streaming, double-buffered with atomics

### No Two Audio Devices
- AlloLib `configureAudio()` opens the single `AudioIO` device
- Spatial Root's `RealtimeBackend` is created with `noDeviceMode=true` — it never touches `AudioIO`
- All spatial rendering happens inside the host-provided callback buffer

---

## 6. Key Constraints Met

| Constraint | Status |
|------------|--------|
| `adm-allo-player` fully removed | ✅ |
| Spatial Root GUI not launched | ✅ |
| Single audio device owner (AlloLib) | ✅ |
| Visual host (shader sphere) kept intact | ✅ |
| No duplicate speaker mapping logic | ✅ |
| Realtime-safe audio callback | ✅ |
| Clear error handling on config failure | ✅ |

---

## 7. Known Limitations

- Uses `al::App` instead of `al::DistributedAppWithState` (AlloSphere distributed mode needs `al_ext` which was removed)
- Scene and layout paths are hardcoded — no runtime file browser
- No on-screen text overlay yet (keyboard-only parameter feedback via stdout)
- Placeholder layout JSON — requires real AlloSphere speaker layout data
- `cpptoml` and `rtmidi` include paths are manually specified in CMake (cult-allolib doesn't add them by default when DOCS/MIDI are enabled)
- `al_ControlNav.cpp` compiled manually (not in any cult-allolib library target)
