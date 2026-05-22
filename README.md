# immersive-allo-root

An AlloLib immersive media application that embeds **Spatial Root** as its spatial audio runtime. Combines distributed shader rendering with layout-aware spatial audio playback, demonstrating how Spatial Root can operate inside an existing host application rather than only as a standalone GUI.

## What It Does

- **Shader Sphere** — Distributed shader rendering on an icosphere (AlloLib DistributedApp).
- **Embedded Spatial Root** — LUSID scene playback through Spatial Root's EngineSession, driven by a speaker layout JSON.
- **Host-Owned Audio** — The AlloLib app owns the audio device and callback; Spatial Root renders into the host-provided buffer.
- **On-Screen Parameters** — Master gain, DBAP focus, speaker/sub mix, elevation mode controlled via keyboard.

## How It Differs From `allo-i-players`

The earlier `allo-i-players` was a static multichannel file player using `adm-allo-player` for pre-decoded 54.1-channel ADM playback with fixed AlloSphere channel mapping.

`immersive-allo-root` removes `adm-allo-player` entirely. Audio playback is handled by an embedded Spatial Root `EngineSession` that loads LUSID scene data and a speaker layout JSON, rendering spatial audio at runtime rather than playing pre-decoded multichannel files.

## Dependencies

- [Spatial Root](https://github.com/Cult-DSP/spatialroot) (submodule, `devel` branch)
- [miniShader](https://github.com/lucianpar/miniShader) (submodule, shader utilities)

Spatial Root vendors its own AlloLib fork (`cult-allolib`) with all features needed by this app.

## Build

```bash
# Clone with submodules
git clone --recursive https://github.com/lucianpar/immersive-allo-root
cd immersive-allo-root

# Initialize submodules if not done during clone
./init.sh

# Configure and build
mkdir -p build/Release
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release -S .
cmake --build build/Release -j

# Run
cd bin
./immersive-allo-root
```

## Asset Setup

Place your Spatial Root scene and layout assets in:

```
assets/scenes/     → LUSID scene JSON files (e.g. example.scene.lusid.json)
assets/layouts/    → Speaker layout JSON files (e.g. allosphere.layout.json)
```

The app loads paths configured at the top of `src/main.cpp`.

## Keyboard Controls

| Key | Action |
|-----|--------|
| `Space` | Toggle play/pause for audio and visuals |
| `1`–`9` | Select shader / visual mode |
| `g` / `G` | Master gain down / up (1 dB steps) |
| `f` / `F` | DBAP focus down / up (multiplicative 0.8× / 1.25×) |
| `s` / `S` | Sub mix down / up (1 dB steps) |
| `m` / `M` | Speaker mix down / up (1 dB steps) |
| `e` | Cycle elevation mode (0=RescaleAtmosUp, 1=RescaleFullSphere, 2=Clamp) |
| `h` | Toggle overlay help |

## Architecture

```
AlloLib DistributedApp
├── Shader sphere / visual layer
├── on-screen Spatial Root controls (keyboard)
├── Host-owned AudioIO (configured in main)
└── SpatialRootAlloHost
    └── Embedded EngineSession
        ├── LUSID scene input (assets/scenes/)
        ├── Layout JSON (assets/layouts/)
        └── Renders into host audio callback
```

The key integration point is `src/SpatialRootAlloHost.hpp`, which wraps `EngineSession` and provides `renderAudio(al::AudioIOData& io)` for use from `onSound`.

## Spatial Root Embedding Mode

Unlike the Spatial Root standalone GUI (which owns its own window, audio device, and lifecycle), `immersive-allo-root` uses **embedding mode**:

- Host (AlloLib) owns the window, graphics, UI, lifecycle, and audio device.
- Spatial Root owns scene loading, layout interpretation, runtime parameters, and spatial rendering.
- Audio is rendered via `EngineSession::renderExternal()`, added specifically for this embedding use case.

## Known Limitations

- Scene and layout paths are hardcoded — no file browser yet.
- Only Spatial Root engine mode (no offline renderer or CULT transcoding).
- Placeholder layout JSON — replace with real AlloSphere layout data.
- Requires Spatial Root engine to support `prepareForExternalAudioCallback()` (added for this project).

## License

Same as the original AlloLib-based project.
