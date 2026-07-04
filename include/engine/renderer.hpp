/**
 * @file renderer.hpp
 * @brief Public API for the Engine rendering subsystem.
 *
 * The rendering subsystem provides OpenGL 3.3 Core Profile abstractions for
 * shader management, draw submission, post-processing, and render pipelines.
 *
 * Key classes:
 * - `Renderer` — owns the render state, viewport, and draw calls.
 * - `Shader` — loads, compiles, and links GLSL programs from files.
 * - `RenderPipeline` — orchestrates full-frame rendering passes.
 * - `PostProcessor` — handles bloom, tonemapping, blur, and overlay composition.
 * - `ShaderLibrary` — caches and manages loaded shader programs by key.
 *
 * @see Renderer
 * @see Shader
 * @see RenderPipeline
 * @see PostProcessor
 * @see ShaderLibrary
 */

#pragma once

#include "core/Renderer.h"
#include "core/Shader.h"
#include "core/RenderPipeline.h"
#include "core/PostProcessor.h"
#include "core/ShaderLibrary.h"
