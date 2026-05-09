#pragma once

#include "world/TestWorld.h"

namespace engine
{
Scene createDaylightSandboxWorld(AtmosphericWorldSettings& worldSettings,
                                 AtmosphericRenderSettings& renderSettings,
                                 const TestWorldAssets& assets);
void syncDaylightSandboxWorld(Scene& scene, AtmosphericWorldSettings& worldSettings,
                              const TestWorldAssets& assets);
void updateDaylightSandboxLighting(Scene& scene, const AtmosphericWorldSettings& worldSettings,
                                   AtmosphericRenderSettings& renderSettings, float timeSeconds);
void syncDaylightSandboxSunVisual(Scene& scene, const AtmosphericRenderSettings& renderSettings,
                                  AtmosphericRuntimeState& runtimeState,
                                  const Vec3& cameraPosition);
} // namespace engine
