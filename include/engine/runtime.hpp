/**
 * @file runtime.hpp
 * @brief Public API for the Engine runtime system.
 *
 * The runtime system manages application modes (scenes, menus, sandboxes) and
 * transitions between them. Each `RuntimeMode` encapsulates a self-contained
 * experience with its own update/render/debug-ui lifecycle.
 *
 * Use `RuntimeFactory` to register named runtimes and instantiate them by `RuntimeId`.
 *
 * @see RuntimeMode
 * @see RuntimeFactory
 * @see RuntimeIds.h
 */

#pragma once

#include "runtime/RuntimeIds.h"
#include "runtime/RuntimeMode.h"
#include "runtime/RuntimeFactory.h"
