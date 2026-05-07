cmake_minimum_required(VERSION 3.20)

project(EngineStarter VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

function(engine_apply_target_defaults target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(MINGW)
        target_link_options(${target_name} PRIVATE -static-libgcc -static-libstdc++)
    endif()
endfunction()

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" ENGINE_SOURCE_DIR_NORMALIZED)

set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/glfw")
set(GLAD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/glad/include")
set(GLAD_SOURCE_FILE "${CMAKE_SOURCE_DIR}/external/glad/src/glad.c")
set(IMGUI_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/imgui")

if(NOT EXISTS "${GLFW_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "GLFW was not found in external/glfw. Follow external/README.md before configuring the project.")
endif()

if(NOT EXISTS "${GLAD_SOURCE_FILE}" OR NOT EXISTS "${GLAD_INCLUDE_DIR}/glad/glad.h")
    message(FATAL_ERROR "GLAD was not found in external/glad. Follow external/README.md before configuring the project.")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EXISTS "${IMGUI_SOURCE_DIR}/imgui.h")
    message(FATAL_ERROR "Dear ImGui was not found in external/imgui. It is required for debug UI builds.")
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("${GLFW_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/external/glfw")

add_library(glad STATIC "${GLAD_SOURCE_FILE}")
target_include_directories(glad PUBLIC "${GLAD_INCLUDE_DIR}")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND EXISTS "${IMGUI_SOURCE_DIR}/imgui.h")
    add_library(imgui STATIC
        ${IMGUI_SOURCE_DIR}/imgui.cpp
        ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
        ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
        ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui PUBLIC
        ${IMGUI_SOURCE_DIR}
        ${IMGUI_SOURCE_DIR}/backends
        ${GLAD_INCLUDE_DIR}
    )
    target_compile_definitions(imgui PUBLIC GLFW_INCLUDE_NONE)
    target_link_libraries(imgui PUBLIC glfw glad OpenGL::GL)
endif()

find_package(OpenGL REQUIRED)

set(ENGINE_SOURCES
    src/main.cpp
    src/Application.cpp
    src/core/Log.cpp
    src/core/Renderer.cpp
    src/core/Shader.cpp
    src/core/ShaderLibrary.cpp
    src/core/FullScreenPass.cpp
    src/core/PostProcessor.cpp
    src/core/RayEvaluationPass.cpp
    src/core/ShadowMapPass.cpp
    src/geometry/Geometry.cpp
    src/geometry/Generators.cpp
    src/geometry/Operations.cpp
    src/graphics/VertexBuffer.cpp
    src/graphics/IndexBuffer.cpp
    src/graphics/VertexArray.cpp
    src/graphics/Mesh.cpp
    src/primitives/PrimitiveBuilders.cpp
    src/primitives/Cube.cpp
    src/primitives/Quad.cpp
    src/primitives/Plane.cpp
    src/primitives/Triangle.cpp
    src/primitives/Pyramid.cpp
    src/primitives/Sphere.cpp
    src/primitives/Cylinder.cpp
    src/primitives/Cone.cpp
    src/primitives/Capsule.cpp
    src/primitives/Torus.cpp
    src/math/Types.cpp
    src/math/Transform.cpp
    src/world/Camera.cpp
    src/world/FreeCameraController.cpp
    src/world/TestWorld.cpp
)

set(ENGINE_HEADERS
    src/Application.h
    src/core/Log.h
    src/core/Renderer.h
    src/core/Shader.h
    src/core/ShaderLibrary.h
    src/core/FullScreenPass.h
    src/core/PostProcessor.h
    src/core/RayEvaluationPass.h
    src/core/ShadowMapPass.h
    src/geometry/Geometry.h
    src/geometry/Generators.h
    src/geometry/Operations.h
    src/graphics/VertexBuffer.h
    src/graphics/IndexBuffer.h
    src/graphics/VertexArray.h
    src/graphics/Mesh.h
    src/primitives/PrimitiveBuilders.h
    src/primitives/Cube.h
    src/primitives/Quad.h
    src/primitives/Plane.h
    src/primitives/Triangle.h
    src/primitives/Pyramid.h
    src/primitives/Sphere.h
    src/primitives/Cylinder.h
    src/primitives/Cone.h
    src/primitives/Capsule.h
    src/primitives/Torus.h
    src/math/Types.h
    src/math/Transform.h
    src/world/Camera.h
    src/world/FreeCameraController.h
    src/world/Lighting.h
    src/world/Material.h
    src/world/RayTracing.h
    src/world/Scene.h
    src/world/TestWorld.h
)

set(ENGINE_SHADERS
    shaders/vertex.glsl
    shaders/fragment.glsl
    shaders/post_blur.vert
    shaders/post_blur.frag
    shaders/ray_eval.vert
    shaders/ray_eval.frag
    shaders/post_tonemap.vert
    shaders/post_tonemap.frag
    shaders/post_compose.frag
    shaders/shadow_depth.vert
    shaders/shadow_depth.frag
)

set(ENGINE_DEBUG_UI_SOURCES)
set(ENGINE_DEBUG_UI_HEADERS)

if(TARGET imgui)
    list(APPEND ENGINE_DEBUG_UI_SOURCES
        src/debug/DebugUi.cpp
    )
    list(APPEND ENGINE_DEBUG_UI_HEADERS
        src/debug/DebugUi.h
    )
endif()

set_source_files_properties(${ENGINE_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(EngineStarter
    ${ENGINE_SOURCES}
    ${ENGINE_DEBUG_UI_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_DEBUG_UI_HEADERS}
    ${ENGINE_SHADERS}
)

target_include_directories(EngineStarter
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${GLAD_INCLUDE_DIR}"
)

target_compile_definitions(EngineStarter
    PRIVATE
        GLFW_INCLUDE_NONE
        ENGINE_SOURCE_DIR="${ENGINE_SOURCE_DIR_NORMALIZED}"
)

target_link_libraries(EngineStarter
    PRIVATE
        glad
        glfw
        OpenGL::GL
)

if(TARGET imgui)
    target_link_libraries(EngineStarter PRIVATE imgui)
    target_compile_definitions(EngineStarter PRIVATE ENGINE_ENABLE_DEBUG_UI=1)
endif()

engine_apply_target_defaults(EngineStarter)

set_target_properties(glad PROPERTIES FOLDER "external")
set_target_properties(glfw PROPERTIES FOLDER "external")
set_target_properties(EngineStarter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:EngineStarter>"
)

set(ENGINE_SHADER_COPY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EngineStarter>/shaders"
)

foreach(shader_file IN LISTS ENGINE_SHADERS)
    get_filename_component(shader_name "${shader_file}" NAME)
    list(APPEND ENGINE_SHADER_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/${shader_file}"
                "$<TARGET_FILE_DIR:EngineStarter>/shaders/${shader_name}"
    )
endforeach()

add_custom_target(EngineStarterShaders ALL
    ${ENGINE_SHADER_COPY_COMMANDS}
    DEPENDS ${ENGINE_SHADERS}
    COMMENT "Copying shader assets"
)

add_dependencies(EngineStarter EngineStarterShaders)

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_SHADERS}
)

if(BUILD_TESTING)
    add_executable(EngineStarterTests
        tests/SmokeTests.cpp
    )

    set_target_properties(EngineStarterTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    target_compile_definitions(EngineStarterTests
        PRIVATE
            ENGINE_SOURCE_DIR="${ENGINE_SOURCE_DIR_NORMALIZED}"
    )

    engine_apply_target_defaults(EngineStarterTests)

    add_test(NAME shader_assets_exist COMMAND EngineStarterTests shader_assets_exist)
    add_test(NAME dependency_layout_exists COMMAND EngineStarterTests dependency_layout_exists)
    add_test(NAME engine_source_layout_exists COMMAND EngineStarterTests engine_source_layout_exists)

    set_tests_properties(
        shader_assets_exist
        dependency_layout_exists
        engine_source_layout_exists
        PROPERTIES
            LABELS "smoke"
    )

    source_group(TREE "${CMAKE_SOURCE_DIR}" FILES tests/SmokeTests.cpp)
endif()
