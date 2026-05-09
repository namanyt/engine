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
    src/metassets/LoadingScene.metasset.cpp
    src/metassets/Metasset.cpp
    src/metassets/MenuScene.metasset.cpp
    src/metassets/SceneMetasset.cpp
    src/metassets/DaylightSandboxScene.metasset.cpp
    src/metassets/TestWorldScene.metasset.cpp
    src/runtime/LoadingRuntime.cpp
    src/runtime/MenuRuntime.cpp
    src/runtime/RuntimeMode.cpp
    src/runtime/RuntimeIds.cpp
    src/runtime/RuntimeFactory.cpp
    src/runtime/ExplorationRuntime.cpp
    src/runtime/InteractionPromptRenderable.cpp
    src/runtime/SettingsOverlay.cpp
    src/runtime/StartupFlowOverlay.cpp
    src/runtime/VnScript.cpp
    src/runtime/VNRuntime.cpp
    src/runtime/SceneRuntime.cpp
    src/runtime/SceneAssetScope.cpp
    src/runtime/EngineRuntime.cpp
    src/scenes/LoadingScene.cpp
    src/scenes/MenuScene.cpp
    src/scenes/DaylightSandboxScene.cpp
    src/scenes/TestWorldScene.cpp
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
)

set(ENGINE_HEADERS
    src/Application.h
    src/metassets/LoadingScene.metasset.h
    src/metassets/Metasset.h
    src/metassets/MenuScene.metasset.h
    src/metassets/SceneMetasset.h
    src/metassets/DaylightSandboxScene.metasset.h
    src/metassets/TestWorldScene.metasset.h
    src/runtime/ExplorationInput.h
    src/runtime/RuntimeIds.h
    src/runtime/RuntimeFactory.h
    src/runtime/RuntimeMode.h
    src/runtime/ExplorationRuntime.h
    src/runtime/InteractionPromptRenderable.h
    src/runtime/LoadingRuntime.h
    src/runtime/MenuRuntime.h
    src/runtime/SettingsOverlay.h
    src/runtime/StartupFlowOverlay.h
    src/runtime/VnScript.h
    src/runtime/VNRuntime.h
    src/runtime/SceneRuntime.h
    src/runtime/SceneAssetScope.h
    src/runtime/EngineRuntime.h
    src/scenes/LoadingScene.h
    src/scenes/MenuScene.h
    src/scenes/AtmosphericSceneRuntime.h
    src/scenes/DaylightSandboxScene.h
    src/scenes/TestWorldScene.h
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
)

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

set_source_files_properties(${ENGINE_SHADERS} ${ENGINE_ASSET_PLACEHOLDERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(EngineStarter
    ${ENGINE_SOURCES}
    ${ENGINE_DEBUG_UI_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_DEBUG_UI_HEADERS}
    ${ENGINE_SHADERS}
    ${ENGINE_ASSET_PLACEHOLDERS}
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

if(WIN32)
    target_link_libraries(EngineStarter PRIVATE ole32 windowscodecs Msimg32)
endif()

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

add_custom_target(EngineStarterAssets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EngineStarter>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:EngineStarter>/assets"
    DEPENDS ${ENGINE_SHADERS} ${ENGINE_ASSET_PLACEHOLDERS}
    COMMENT "Copying engine assets"
)

add_dependencies(EngineStarter EngineStarterAssets)
engine_stage_runtime_dependencies(EngineStarter)

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${ENGINE_SHADERS}
    ${ENGINE_ASSET_PLACEHOLDERS}
)

if(BUILD_TESTING)
    add_executable(EngineStarterTests
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
        src/math/Types.cpp
    )

    set_target_properties(EngineStarterTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    engine_stage_runtime_dependencies(EngineStarterTests)

    target_include_directories(EngineStarterTests
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
            "${GLAD_INCLUDE_DIR}"
    )

    target_link_libraries(EngineStarterTests
        PRIVATE
            glad
            OpenGL::GL
    )

    if(WIN32)
        target_link_libraries(EngineStarterTests PRIVATE ole32 windowscodecs)
    endif()

    target_compile_definitions(EngineStarterTests
        PRIVATE
            ENGINE_SOURCE_DIR="${ENGINE_SOURCE_DIR_NORMALIZED}"
    )

    engine_apply_target_defaults(EngineStarterTests)

    add_test(NAME shader_assets_exist COMMAND EngineStarterTests shader_assets_exist)
    add_test(NAME asset_directory_layout_exists COMMAND EngineStarterTests asset_directory_layout_exists)
    add_test(NAME dependency_layout_exists COMMAND EngineStarterTests dependency_layout_exists)
    add_test(NAME engine_source_layout_exists COMMAND EngineStarterTests engine_source_layout_exists)
    add_test(NAME asset_manager_round_trip COMMAND EngineStarterTests asset_manager_round_trip)
    add_test(NAME vnscript_parser_smoke COMMAND EngineStarterTests vnscript_parser_smoke)

    set_tests_properties(
        shader_assets_exist
        asset_directory_layout_exists
        dependency_layout_exists
        engine_source_layout_exists
        asset_manager_round_trip
        vnscript_parser_smoke
        PROPERTIES
            LABELS "smoke"
    )

    source_group(TREE "${CMAKE_SOURCE_DIR}" FILES tests/SmokeTests.cpp)
endif()
