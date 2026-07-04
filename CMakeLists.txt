cmake_minimum_required(VERSION 3.20)

project(EngineStarter VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_ATMOSPHERIC_DEMO "Build atmospheric demo example" ON)

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

function(engine_stage_runtime_dependencies target_name)
    if(NOT MINGW)
        return()
    endif()

    get_filename_component(engine_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(runtime_copy_script_content [=[
set(target_file "$<TARGET_FILE:@target_name@>")
set(target_dir "$<TARGET_FILE_DIR:@target_name@>")

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${target_file}"
    RESOLVED_DEPENDENCIES_VAR resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
    DIRECTORIES "@engine_mingw_bin_dir@"
    PRE_EXCLUDE_REGEXES "api-ms-win-.*" "ext-ms-win-.*"
    POST_EXCLUDE_REGEXES ".*[/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\].*"
)

foreach(resolved_dependency IN LISTS resolved_dependencies)
    file(COPY "${resolved_dependency}" DESTINATION "${target_dir}")
endforeach()

if(unresolved_dependencies)
    list(JOIN unresolved_dependencies ", " unresolved_dependency_list)
    message(WARNING "Unresolved runtime dependencies for @target_name@: ${unresolved_dependency_list}")
endif()
]=])

    string(CONFIGURE "${runtime_copy_script_content}" runtime_copy_script_content @ONLY)

    set(runtime_copy_script "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_stage_runtime_deps.cmake")
    file(GENERATE OUTPUT "${runtime_copy_script}" CONTENT "${runtime_copy_script_content}")

    add_custom_command(
        TARGET ${target_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -P "${runtime_copy_script}"
        VERBATIM
    )
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
 set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

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

# ============================================================================
# Engine Library
# ============================================================================

set(ENGINE_SOURCES
    src/Application.cpp
    src/core/AudioSystem.cpp
    src/assets/Asset.cpp
    src/assets/AudioAsset.cpp
    src/assets/AssetManager.cpp
    src/assets/AssetMeta.cpp
    src/assets/AssetRegistry.cpp
    src/assets/ModelAsset.cpp
    src/assets/ShaderAsset.cpp
    src/assets/TextureAsset.cpp
    src/core/Log.cpp
    src/core/RenderDebug.cpp
    src/core/RenderProfiler.cpp
    src/core/Renderer.cpp
    src/core/Shader.cpp
    src/core/ShaderLibrary.cpp
    src/core/FullScreenPass.cpp
    src/core/PostProcessor.cpp
    src/core/RenderPipeline.cpp
    src/core/RayEvaluationPass.cpp
    src/core/ShadowMapPass.cpp
    src/geometry/Geometry.cpp
    src/geometry/Generators.cpp
    src/geometry/ObjParser.cpp
    src/geometry/Operations.cpp
    src/graphics/VertexBuffer.cpp
    src/graphics/IndexBuffer.cpp
    src/graphics/VertexArray.cpp
    src/graphics/Mesh.cpp
    src/systems/TransformSystem.cpp
    src/systems/RenderSystem.cpp
    src/systems/InteractionSystem.cpp
    src/systems/WorldEcsSystems.cpp
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
    src/world/Player.cpp
    src/world/PlayerController.cpp
    src/world/Scene.cpp
    src/world/DaylightSandbox.cpp
    src/world/TestWorld.cpp
    src/world/WorldNavigation.cpp
    src/runtime/RuntimeMode.cpp
    src/runtime/RuntimeIds.cpp
    src/runtime/RuntimeFactory.cpp
)

set(ENGINE_HEADERS
    src/Application.h
    src/core/AudioSystem.h
    src/core/AudioCategory.h
    src/assets/Asset.h
    src/assets/AudioAsset.h
    src/assets/AssetHandle.h
    src/assets/AssetManager.h
    src/assets/AssetMeta.h
    src/assets/AssetRegistry.h
    src/assets/ModelAsset.h
    src/assets/ShaderAsset.h
    src/assets/TextureAsset.h
    src/core/Log.h
    src/core/RenderDebug.h
    src/core/RenderProfiler.h
    src/core/Renderer.h
    src/core/Shader.h
    src/core/ShaderLibrary.h
    src/core/FullScreenPass.h
    src/core/PostProcessor.h
    src/core/RenderPipeline.h
    src/core/RayEvaluationPass.h
    src/core/ShadowMapPass.h
    src/geometry/Geometry.h
    src/geometry/Generators.h
    src/geometry/ObjParser.h
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
    src/ecs/Entity.h
    src/ecs/Registry.h
    src/components/WorldComponents.h
    src/systems/TransformSystem.h
    src/systems/RenderSystem.h
    src/systems/InteractionSystem.h
    src/systems/WorldEcsSystems.h
    src/world/Camera.h
    src/world/FreeCameraController.h
    src/world/Lighting.h
    src/world/Material.h
    src/world/Player.h
    src/world/PlayerController.h
    src/world/RayTracing.h
    src/world/Scene.h
    src/world/DaylightSandbox.h
    src/world/TestWorld.h
    src/world/WorldNavigation.h
    src/runtime/RuntimeMode.h
    src/runtime/RuntimeIds.h
    src/runtime/RuntimeFactory.h
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

add_library(engine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_DEBUG_UI_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_DEBUG_UI_HEADERS}
)

target_include_directories(engine
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src"
        "${GLAD_INCLUDE_DIR}"
        "${CMAKE_SOURCE_DIR}/external/miniaudio"
)

target_compile_definitions(engine
    PRIVATE
        GLFW_INCLUDE_NONE
)

if(TARGET imgui)
    target_compile_definitions(engine PUBLIC ENGINE_ENABLE_DEBUG_UI=1)
endif()

target_link_libraries(engine
    PUBLIC
        glad
        glfw
        OpenGL::GL
)

if(WIN32)
    target_link_libraries(engine PRIVATE ole32 windowscodecs Msimg32)
endif()

if(TARGET imgui)
    target_link_libraries(engine PRIVATE imgui)
endif()

engine_apply_target_defaults(engine)

set_target_properties(engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    FOLDER "engine"
)

# ============================================================================
# Shader and Asset Files
# ============================================================================

set(ENGINE_SHADERS
    assets/shaders/vertex.glsl
    assets/shaders/fragment.glsl
    assets/shaders/post_blur.vert
    assets/shaders/post_blur.frag
    assets/shaders/ui_overlay.frag
    assets/shaders/ray_eval.vert
    assets/shaders/ray_eval.frag
    assets/shaders/post_tonemap.vert
    assets/shaders/post_tonemap.frag
    assets/shaders/post_compose.frag
    assets/shaders/shadow_depth.vert
    assets/shaders/shadow_depth.frag
    assets/shaders/interaction_prompt.vert
    assets/shaders/interaction_prompt.frag
    assets/shaders/volumetric_upscale.frag
)

set(ENGINE_ASSET_PLACEHOLDERS
    assets/textures/.gitkeep
    assets/audio/.gitkeep
    assets/models/.gitkeep
    assets/videos/.gitkeep
    assets/fonts/.gitkeep
    assets/scripts/test.vnscript
    assets/test/.gitkeep
)

set_source_files_properties(${ENGINE_SHADERS} ${ENGINE_ASSET_PLACEHOLDERS} PROPERTIES HEADER_FILE_ONLY TRUE)

# ============================================================================
# Example Applications
# ============================================================================

if(BUILD_EXAMPLES AND BUILD_ATMOSPHERIC_DEMO)
    add_executable(atmospheric_demo
        examples/atmospheric_demo/src/main.cpp
        examples/atmospheric_demo/src/EngineRuntime.cpp
        examples/atmospheric_demo/src/EngineRuntime.h
        examples/atmospheric_demo/src/MenuRuntime.cpp
        examples/atmospheric_demo/src/MenuRuntime.h
        examples/atmospheric_demo/src/ExplorationRuntime.cpp
        examples/atmospheric_demo/src/ExplorationRuntime.h
        examples/atmospheric_demo/src/VNRuntime.cpp
        examples/atmospheric_demo/src/VNRuntime.h
        examples/atmospheric_demo/src/LoadingRuntime.cpp
        examples/atmospheric_demo/src/LoadingRuntime.h
        examples/atmospheric_demo/src/InteractionPromptRenderable.cpp
        examples/atmospheric_demo/src/InteractionPromptRenderable.h
        examples/atmospheric_demo/src/SettingsPage.cpp
        examples/atmospheric_demo/src/SettingsPage.h
        examples/atmospheric_demo/src/SettingsOverlay.cpp
        examples/atmospheric_demo/src/SettingsOverlay.h
        examples/atmospheric_demo/src/StartupFlowOverlay.cpp
        examples/atmospheric_demo/src/StartupFlowOverlay.h
        examples/atmospheric_demo/src/VnScript.cpp
        examples/atmospheric_demo/src/VnScript.h
        examples/atmospheric_demo/src/SceneRuntime.cpp
        examples/atmospheric_demo/src/SceneRuntime.h
        examples/atmospheric_demo/src/SceneAssetScope.cpp
        examples/atmospheric_demo/src/SceneAssetScope.h
        examples/atmospheric_demo/src/ExplorationInput.h
        examples/atmospheric_demo/src/LoadingScene.metasset.cpp
        examples/atmospheric_demo/src/LoadingScene.metasset.h
        examples/atmospheric_demo/src/Metasset.cpp
        examples/atmospheric_demo/src/Metasset.h
        examples/atmospheric_demo/src/MenuScene.metasset.cpp
        examples/atmospheric_demo/src/MenuScene.metasset.h
        examples/atmospheric_demo/src/SceneMetasset.cpp
        examples/atmospheric_demo/src/SceneMetasset.h
        examples/atmospheric_demo/src/DaylightSandboxScene.metasset.cpp
        examples/atmospheric_demo/src/DaylightSandboxScene.metasset.h
        examples/atmospheric_demo/src/TestWorldScene.metasset.cpp
        examples/atmospheric_demo/src/TestWorldScene.metasset.h
        examples/atmospheric_demo/src/LoadingScene.cpp
        examples/atmospheric_demo/src/LoadingScene.h
        examples/atmospheric_demo/src/MenuScene.cpp
        examples/atmospheric_demo/src/MenuScene.h
        examples/atmospheric_demo/src/AtmosphericSceneRuntime.h
        examples/atmospheric_demo/src/DaylightSandboxScene.cpp
        examples/atmospheric_demo/src/DaylightSandboxScene.h
        examples/atmospheric_demo/src/TestWorldScene.cpp
        examples/atmospheric_demo/src/TestWorldScene.h
    )

    target_include_directories(atmospheric_demo
        PRIVATE
            "${CMAKE_SOURCE_DIR}/examples/atmospheric_demo/src"
    )

    target_compile_definitions(atmospheric_demo
        PRIVATE
            GLFW_INCLUDE_NONE
            ENGINE_SOURCE_DIR="${ENGINE_SOURCE_DIR_NORMALIZED}"
    )

    target_link_libraries(atmospheric_demo
        PRIVATE
            engine
    )

    engine_apply_target_defaults(atmospheric_demo)

    set_target_properties(atmospheric_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:atmospheric_demo>"
    )

    # Copy assets to example output directory
    add_custom_target(atmospheric_demo_assets ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:atmospheric_demo>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:atmospheric_demo>/assets"
        DEPENDS ${ENGINE_SHADERS} ${ENGINE_ASSET_PLACEHOLDERS}
        COMMENT "Copying assets for atmospheric demo"
    )

    add_dependencies(atmospheric_demo atmospheric_demo_assets)
    engine_stage_runtime_dependencies(atmospheric_demo)

    source_group(TREE "${CMAKE_SOURCE_DIR}" FILES examples/atmospheric_demo/src/*.cpp examples/atmospheric_demo/src/*.h)
endif()

# ============================================================================
# Tests
# ============================================================================

if(BUILD_TESTING)
    # ========================================================================
    # Smoke Tests - Infrastructure and asset smoke tests
    # ========================================================================

    add_executable(SmokeTests
        tests/SmokeTests.cpp
        src/runtime/VnScript.cpp
        src/assets/Asset.cpp
        src/assets/AudioAsset.cpp
        src/assets/AssetManager.cpp
        src/assets/AssetMeta.cpp
        src/assets/AssetRegistry.cpp
        src/assets/ModelAsset.cpp
        src/assets/ShaderAsset.cpp
        src/assets/TextureAsset.cpp
        src/core/RenderDebug.cpp
        src/core/Log.cpp
        src/core/Shader.cpp
        src/core/ShaderLibrary.cpp
        src/geometry/Geometry.cpp
        src/geometry/ObjParser.cpp
        src/math/Types.cpp
        src/math/Transform.cpp
    )

    set_target_properties(SmokeTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    engine_stage_runtime_dependencies(SmokeTests)

    target_include_directories(SmokeTests
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
            "${GLAD_INCLUDE_DIR}"
            "${CMAKE_SOURCE_DIR}/external/miniaudio"
    )

    target_link_libraries(SmokeTests
        PRIVATE
            glad
            OpenGL::GL
    )

    if(WIN32)
        target_link_libraries(SmokeTests PRIVATE ole32 windowscodecs)
    endif()

    target_compile_definitions(SmokeTests
        PRIVATE
            ENGINE_SOURCE_DIR="${ENGINE_SOURCE_DIR_NORMALIZED}"
    )

    engine_apply_target_defaults(SmokeTests)

    add_test(NAME shader_assets_exist COMMAND SmokeTests shader_assets_exist)
    add_test(NAME asset_directory_layout_exists COMMAND SmokeTests asset_directory_layout_exists)
    add_test(NAME dependency_layout_exists COMMAND SmokeTests dependency_layout_exists)
    add_test(NAME asset_manager_round_trip COMMAND SmokeTests asset_manager_round_trip)
    add_test(NAME vnscript_parser_smoke COMMAND SmokeTests vnscript_parser_smoke)
    add_test(NAME audio_settings_contract COMMAND SmokeTests audio_settings_contract)

    set_tests_properties(
        shader_assets_exist
        asset_directory_layout_exists
        dependency_layout_exists
        asset_manager_round_trip
        vnscript_parser_smoke
        audio_settings_contract
        PROPERTIES
            LABELS "smoke"
    )

    # ========================================================================
    # Math Tests - Vector, matrix, and transform unit tests
    # ========================================================================

    add_executable(MathTests
        tests/MathTests.cpp
        src/math/Types.cpp
        src/math/Transform.cpp
    )

    set_target_properties(MathTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    engine_stage_runtime_dependencies(MathTests)

    target_include_directories(MathTests
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
    )

    engine_apply_target_defaults(MathTests)

    add_test(NAME math_vec2_addition COMMAND MathTests math_vec2_addition)
    add_test(NAME math_vec2_subtraction COMMAND MathTests math_vec2_subtraction)
    add_test(NAME math_vec2_scalar_multiply COMMAND MathTests math_vec2_scalar_multiply)
    add_test(NAME math_vec2_scalar_divide COMMAND MathTests math_vec2_scalar_divide)
    add_test(NAME math_vec2_dot COMMAND MathTests math_vec2_dot)
    add_test(NAME math_vec2_length COMMAND MathTests math_vec2_length)
    add_test(NAME math_vec2_normalize COMMAND MathTests math_vec2_normalize)
    add_test(NAME math_vec3_addition COMMAND MathTests math_vec3_addition)
    add_test(NAME math_vec3_subtraction COMMAND MathTests math_vec3_subtraction)
    add_test(NAME math_vec3_negation COMMAND MathTests math_vec3_negation)
    add_test(NAME math_vec3_scalar_multiply COMMAND MathTests math_vec3_scalar_multiply)
    add_test(NAME math_vec3_scalar_divide COMMAND MathTests math_vec3_scalar_divide)
    add_test(NAME math_vec3_dot COMMAND MathTests math_vec3_dot)
    add_test(NAME math_vec3_cross COMMAND MathTests math_vec3_cross)
    add_test(NAME math_vec3_length COMMAND MathTests math_vec3_length)
    add_test(NAME math_vec3_normalize COMMAND MathTests math_vec3_normalize)
    add_test(NAME math_vec4_addition COMMAND MathTests math_vec4_addition)
    add_test(NAME math_vec4_scalar_multiply COMMAND MathTests math_vec4_scalar_multiply)
    add_test(NAME math_color_white COMMAND MathTests math_color_white)
    add_test(NAME math_color_black COMMAND MathTests math_color_black)
    add_test(NAME math_mat4_identity COMMAND MathTests math_mat4_identity)
    add_test(NAME math_mat4_multiplication COMMAND MathTests math_mat4_multiplication)
    add_test(NAME math_mat4_translation COMMAND MathTests math_mat4_translation)
    add_test(NAME math_mat4_scale COMMAND MathTests math_mat4_scale)
    add_test(NAME math_mat4_rotation_x COMMAND MathTests math_mat4_rotation_x)
    add_test(NAME math_mat4_rotation_y COMMAND MathTests math_mat4_rotation_y)
    add_test(NAME math_mat4_rotation_z COMMAND MathTests math_mat4_rotation_z)
    add_test(NAME math_mat4_inverse COMMAND MathTests math_mat4_inverse)
    add_test(NAME math_mat4_perspective COMMAND MathTests math_mat4_perspective)
    add_test(NAME math_mat4_orthographic COMMAND MathTests math_mat4_orthographic)
    add_test(NAME math_mat4_look_at COMMAND MathTests math_mat4_look_at)
    add_test(NAME math_transform_model_matrix COMMAND MathTests math_transform_model_matrix)
    add_test(NAME math_transform_scale COMMAND MathTests math_transform_scale)
    add_test(NAME math_transform_combined COMMAND MathTests math_transform_combined)

    set_tests_properties(
        math_vec2_addition
        math_vec2_subtraction
        math_vec2_scalar_multiply
        math_vec2_scalar_divide
        math_vec2_dot
        math_vec2_length
        math_vec2_normalize
        math_vec3_addition
        math_vec3_subtraction
        math_vec3_negation
        math_vec3_scalar_multiply
        math_vec3_scalar_divide
        math_vec3_dot
        math_vec3_cross
        math_vec3_length
        math_vec3_normalize
        math_vec4_addition
        math_vec4_scalar_multiply
        math_color_white
        math_color_black
        math_mat4_identity
        math_mat4_multiplication
        math_mat4_translation
        math_mat4_scale
        math_mat4_rotation_x
        math_mat4_rotation_y
        math_mat4_rotation_z
        math_mat4_inverse
        math_mat4_perspective
        math_mat4_orthographic
        math_mat4_look_at
        math_transform_model_matrix
        math_transform_scale
        math_transform_combined
        PROPERTIES
            LABELS "math"
    )

    # ========================================================================
    # OBJ Parser Tests - Wavefront OBJ format parsing tests
    # ========================================================================

    add_executable(ObjParserTests
        tests/ObjParserTests.cpp
        src/math/Types.cpp
        src/geometry/Geometry.cpp
        src/geometry/ObjParser.cpp
    )

    set_target_properties(ObjParserTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    engine_stage_runtime_dependencies(ObjParserTests)

    target_include_directories(ObjParserTests
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
    )

    engine_apply_target_defaults(ObjParserTests)

    add_test(NAME obj_parse_simple_triangle COMMAND ObjParserTests obj_parse_simple_triangle)
    add_test(NAME obj_parse_triangle_with_normals COMMAND ObjParserTests obj_parse_triangle_with_normals)
    add_test(NAME obj_parse_triangle_with_uvs COMMAND ObjParserTests obj_parse_triangle_with_uvs)
    add_test(NAME obj_parse_triangle_with_full_refs COMMAND ObjParserTests obj_parse_triangle_with_full_refs)
    add_test(NAME obj_parse_quad COMMAND ObjParserTests obj_parse_quad)
    add_test(NAME obj_parse_pentagon COMMAND ObjParserTests obj_parse_pentagon)
    add_test(NAME obj_parse_empty_input COMMAND ObjParserTests obj_parse_empty_input)
    add_test(NAME obj_parse_comments_only COMMAND ObjParserTests obj_parse_comments_only)
    add_test(NAME obj_parse_negative_indices COMMAND ObjParserTests obj_parse_negative_indices)
    add_test(NAME obj_parse_multiple_faces COMMAND ObjParserTests obj_parse_multiple_faces)
    add_test(NAME obj_parse_ignores_comments_and_whitespace COMMAND ObjParserTests obj_parse_ignores_comments_and_whitespace)
    add_test(NAME obj_parse_cube COMMAND ObjParserTests obj_parse_cube)
    add_test(NAME obj_parse_default_normal COMMAND ObjParserTests obj_parse_default_normal)
    add_test(NAME obj_parse_from_bytes COMMAND ObjParserTests obj_parse_from_bytes)
    add_test(NAME obj_parse_null_input COMMAND ObjParserTests obj_parse_null_input)
    add_test(NAME obj_parse_vertices_without_faces COMMAND ObjParserTests obj_parse_vertices_without_faces)

    set_tests_properties(
        obj_parse_simple_triangle
        obj_parse_triangle_with_normals
        obj_parse_triangle_with_uvs
        obj_parse_triangle_with_full_refs
        obj_parse_quad
        obj_parse_pentagon
        obj_parse_empty_input
        obj_parse_comments_only
        obj_parse_negative_indices
        obj_parse_multiple_faces
        obj_parse_ignores_comments_and_whitespace
        obj_parse_cube
        obj_parse_default_normal
        obj_parse_from_bytes
        obj_parse_null_input
        obj_parse_vertices_without_faces
        PROPERTIES
            LABELS "obj"
    )

    source_group(TREE "${CMAKE_SOURCE_DIR}" FILES tests/SmokeTests.cpp tests/MathTests.cpp tests/ObjParserTests.cpp)
endif()
