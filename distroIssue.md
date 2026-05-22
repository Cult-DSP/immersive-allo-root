# DistributedApp Architecture & Remaining Issues

## What We Have

`ImmersiveAlloRootApp` extends `DistributedAppWithState<CommonState>`, matching the
original `allo-i-players` `immersivePlayer.cpp` pattern:

- Parameters (`globalTime`, `running`, `currentFragIndex`, Spatial Root params)
  registered with `parameterServer()` in `onInit()` → broadcast via OSC to replicas.
- `isPrimary()` guards on keyboard input (`onKeyDown`) and Spatial Root
  initialization (`onCreate` → `setup()`).
- `onSound` is primary-only: primary renders via `SpatialRootAlloHost`, replica
  outputs silence.
- `main()` detects AlloSphere vs desktop via `al::sphere::isSphereMachine()`.

## Known & Suspected Issues

### 1. Missing `#include` for `al/sphere/al_SphereUtils.hpp`

`main()` calls `al::sphere::isSphereMachine()` but does not explicitly include
the header. It may pull it in transitively via `al_DistributedApp.hpp`, but the
dependency is implicit and fragile.

**Fix:** add `#include "al/sphere/al_SphereUtils.hpp"` to `main.cpp`.

### 2. No CuttleboneDomain

The original `shaderDistroRef.cpp` (the more complex distributed reference)
enables `CuttleboneDomain<Common>` for low-latency state struct distribution.
The `DistributedAppWithState<CommonState>` template gives us a typed
`state()` member, but without Cuttlebone it is never synced between instances.

Our current approach matches the simpler `immersivePlayer.cpp` pattern, which
relies solely on `ParameterServer` + registered `Parameter` objects. This works
for the scalar/float parameters we distribute (`running`, `globalTime`,
`currentFragIndex`, `masterGainDb`, etc.) but does **not** synchronise the
`CommonState` struct itself.

**Impact:** fine for current use (all distributed values are `Parameter`
objects). If we ever put data inside `CommonState` (mesh positions, quaternions,
etc.), CuttleboneDomain must be enabled.

### 3. Audio Device Contention (Dual-Instance)

When two instances run via `2run.sh`, both call `configureAudio()` and both
open the host audio device. On macOS / CoreAudio this is usually fine (multiple
clients), but the replica's `onSound` outputs silence — it still occupies
device resources.

**Potential issue on AlloSphere:** if each sphere machine runs one instance and
the primary/replica detection works via OSC, the non-primary machine should
not open the audio device. Currently both do.

**Mitigation:** `DistributedApp::prepare()` line 158-163 already handles this:
```cpp
if (hasCapability(CAP_AUDIO_IO)) {
    mAudioControl.registerAudioIO(audioIO());
} else {
    mDomainList.erase(
        std::find(mDomainList.begin(), mDomainList.end(), mAudioDomain));
}
```
The replica capability is `CAP_STATE_RECEIVE | CAP_OMNIRENDERING` (no
`CAP_AUDIO_IO`), so the audio domain should be removed on replicas. Verify that
this is working correctly in `cult-allolib`.

### 4. `distributed_app.toml` Runtime Path

The `distributed_app.toml` file is expected relative to the process CWD. When
launched from `bin/`, `File::exists("distributed_app.toml")` checks
`bin/distributed_app.toml`. If not found, `TomlLoader::setFile()` creates an
empty one there and parses it → empty table → falls back to desktop mode.

**Status:** this matches the original behavior and should be fine, but the
empty file we placed in the repo root is never used at runtime from `bin/`.

### 5. `onInit()` Calls `isPrimary()` — Timing

`DistributedApp::start()` calls `prepare()` (which probes OSC port and
determines primary/replica) at line 193, then `initializeDomains()` at line
179, then `onInit()` at line 268. So `isPrimary()` is valid inside `onInit()`.

**Status:** verified — timing is correct.

### 6. `onDraw` on Replica

Both primary and replica render the shader sphere. The replica does NOT render
an overlay (none exists yet). The primary needs an overlay for Spatial Root
status/parameter feedback, but this is a feature gap, not a distribution bug.

### 7. Shader Change Propagation on Replica

When `currentFragIndex` changes on primary, the ParameterServer broadcasts via
OSC. The replica's `onAnimate` detects `currentFragIndex.get() != currentFlag`
and calls `shadedSphere.setShaders()`.

**Potential issue:** `onInit()` on the replica does NOT populate
`fragPathOptions` (it runs `searchPaths.find()` which requires files to exist
locally). If the replica machine doesn't have the shader files, this will fail.
On desktop dual-instance this is fine (same filesystem). On the AlloSphere,
each machine would need the shader files.

### 8. `shadedSphere` Not a Distributed Resource

The `ShadedSphere` object is created independently on each instance. The shader
uniforms (`u_time`, etc.) are set from `globalTime` which IS a distributed
parameter. So the visuals should match across instances.

## Next Steps to Verify

1. **Add explicit include** for `al/sphere/al_SphereUtils.hpp` in `main.cpp`.
2. **Test dual-instance** on desktop:
   - Launch `./run.sh` (single instance) — verify standalone works.
   - Launch `./2run.sh` — verify both windows open, primary keyboard works,
     replica shows same shader, replica audio is silent, primary-to-replica
     OSC sync works for param changes.
3. **Verify replica audio domain removal** — check whether `hasCapability(CAP_AUDIO_IO)`
   returns false on replica and `mAudioDomain` is erased.
4. **Shutdown/cleanup** — ensure `SpatialRootAlloHost` shuts down cleanly
   on app exit (no crash when destroying `EngineSession` mid-audio-callback).
5. **Verify `distributed_app.toml` auto-creation** in `bin/` after first launch.
