/**
 * @file assets.hpp
 * @brief Public API for the Engine asset management subsystem.
 *
 * The asset system provides discovery, loading, caching, and type-safe handles
 * for all engine resources (textures, audio, models, shaders, fonts, videos).
 *
 * Key types:
 * - `AssetManager` — central manager for discovery, loading, and caching.
 * - `AssetHandle<T>` — lightweight type-safe handle referencing an asset by UUID.
 * - `AssetMeta` — metadata (UUID, type, path, tags) for a discovered asset.
 * - `Asset` / `TextureAsset` / `AudioAsset` / etc. — typed asset classes.
 *
 * @par Example
 * @code
 * manager.discover("assets/");
 * auto tex = manager.load<TextureAsset>("textures/background.png.meta");
 * @endcode
 *
 * @see AssetManager
 * @see AssetHandle
 * @see AssetMeta
 */

#pragma once

#include "assets/AssetManager.h"
#include "assets/Asset.h"
#include "assets/AudioAsset.h"
#include "assets/ModelAsset.h"
#include "assets/ShaderAsset.h"
#include "assets/TextureAsset.h"
#include "assets/AssetHandle.h"
#include "assets/AssetMeta.h"
