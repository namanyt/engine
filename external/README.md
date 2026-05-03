# External Dependencies

This project keeps third-party libraries outside the engine source tree while still allowing a normal CMake workflow.

## Expected layout

```text
external/
  glfw/
    CMakeLists.txt
    include/
    src/
    ...
  glad/
    include/
      glad/glad.h
      KHR/khrplatform.h
    src/
      glad.c
```

## GLFW setup

1. Download the GLFW source package from https://www.glfw.org/download.html or clone https://github.com/glfw/glfw .
2. Place the extracted source tree in `external/glfw`.
3. Do not build GLFW manually first; the root `CMakeLists.txt` builds it as part of this project.

## GLAD setup

1. Open https://glad.dav1d.de/ .
2. Choose these options:
   - Language: `C/C++`
   - Specification: `OpenGL`
   - API: `gl = 3.3`
   - Profile: `Core`
   - Generate a loader: enabled
3. Generate and download the GLAD package.
4. Copy the generated files so the layout matches this project:
   - `include/glad/glad.h` -> `external/glad/include/glad/glad.h`
   - `include/KHR/khrplatform.h` -> `external/glad/include/KHR/khrplatform.h`
   - `src/glad.c` -> `external/glad/src/glad.c`

## Why this layout

- Keeps engine code independent from vendor source layout changes
- Lets CMake treat GLFW as a normal subproject
- Keeps GLAD explicit and easy to regenerate when upgrading OpenGL targets
