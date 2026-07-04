# TODO

## Completed

- [x] Refactor RuntimeFactory from switch-case to registration pattern
- [x] Create public API headers (engine.hpp, runtime.hpp, renderer.hpp, ecs.hpp, assets.hpp)
- [x] Split project into engine library and atmospheric_demo example
- [x] Update README for engine library documentation
- [x] Remove redundant engine_source_layout_exists test
- [x] Create comprehensive library documentation

## Known Limitations

### glTF Model Parsing Not Implemented

- **Current state**: .gltf files are loaded as raw binary data only
- **Missing**: No glTF scene graph, material, or animation parsing
- **Impact**: glTF models cannot be rendered directly; must use OBJ format instead
- **Planned**: Implement glTF parser after OBJ is stable

### Debug Build Requires ImGui

- Debug builds need ImGui dependency installed
- Release builds work without it

## Next Steps

### Priority: Medium

- [x] Add unit tests for math utilities (vector/matrix operations)
- [ ] Add unit tests for ECS system behavior
- [ ] Add unit tests for asset manager functionality

### Priority: Low

- [ ] Implement glTF model parsing
- [x] Add Doxygen-style comments to public headers
- [ ] Consider adding more example applications

## Current State

### Build Status

- ✅ Release build working (Ninja + MinGW)
- ❌ Debug build requires ImGui dependency
- ✅ All 56 tests passing (6 smoke + 34 math + 16 obj)
- ✅ Wavefront OBJ parsing with fan triangulation, normals, UVs, negative indices

### Project Structure

```
engine-lib/
├── include/engine/          # Public API headers
├── src/                     # Engine implementation
├── examples/atmospheric_demo/  # Example application
├── tests/                   # Unit tests
└── docs/                    # Documentation
    ├── architecture.md      # System architecture overview
    ├── installation.md      # Installation guide
    ├── quickstart/          # Getting started guide
    ├── runtime/             # Runtime system documentation
    ├── renderer/            # Renderer documentation
    ├── ecs/                  # ECS documentation
    ├── assets/               # Asset management documentation
    └── examples/             # Example walkthroughs
```
