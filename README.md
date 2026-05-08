# Engine

A relaxing atmospheric adventure about exploring beautiful worlds, meeting strange people, and discovering what connects them all.

Step into a constantly shifting collection of environments filled with quiet forests, warm sunsets, distant structures, cozy spaces, and hidden places waiting to be explored. Take your time, wander freely, and enjoy a slow-paced experience focused on atmosphere, immersion, and discovery.

Whether you're walking through fog-covered hills, watching moonlight pass over distant landscapes, or simply sitting quietly in places that feel oddly familiar, `Engine` is designed to be an experience you can get lost in.

---

## Features

- Atmospheric first-person exploration
- Stylized low-poly environments
- Dynamic lighting and volumetric atmosphere
- Seamless runtime scene transitions
- Experimental narrative structure
- Hybrid exploration and VN-inspired storytelling
- Minimalist immersive interface
- Procedural environmental variation
- Runtime-driven world systems
- Relaxed exploration-focused pacing

---

## About The Game

Every place in `Engine` has its own personality.

Some are lively and comforting.
Others are quiet and distant.
Some places feel strangely familiar.

As you continue exploring, you'll slowly uncover more about the world, the people connected to it, and the hidden links between places that should never have crossed paths.

Take your time.
There’s no rush.

---

## Technical Overview

`Engine` is built entirely on a custom C++ runtime and rendering framework developed specifically for the game itself.

Current engine/runtime systems include:

- Custom OpenGL 3.3 renderer
- Runtime-mode architecture (`Menu`, `Loading`, `Exploration`)
- Atmospheric volumetric fog pipeline
- HDR post-processing and bloom
- ECS-driven world systems
- Procedural environment generation
- Runtime asset loading and lazy shader compilation
- Metasset-based scene architecture
- Pause and overlay runtime systems
- ImGui-based debug and profiling tools
- Scene-local asset ownership and runtime transitions

The project is intentionally code-authored and runtime-driven rather than editor-centric.

---

## Build

### Configure

```powershell
cmake --preset debug-ninja
```

### Build

```powershell
cmake --build --preset build-debug-ninja
```

### Run Tests

```powershell
ctest --preset test-debug-ninja --output-on-failure
```

---

## Current Development Status

`Engine` is currently in active development.

The current build contains:

- atmospheric exploration gameplay
- runtime scene transitions
- procedural environments
- menu and overlay systems
- interaction foundations
- dynamic atmosphere rendering
- experimental narrative systems

Additional environments, interactions, characters, and systems are still in development.

---

This game is not suitable for children or those who are easily disturbed.
