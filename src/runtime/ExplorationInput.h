#pragma once

#include "math/Types.h"

namespace engine
{
struct ExplorationInputState final
{
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool crouch = false;
    bool jump = false;
    bool sprint = false;
    bool interact = false;
    bool toggleDebugFreeCamera = false;
    bool toggleMoonLight = false;
    bool toggleSphereLights = false;
    bool toggleConeLights = false;
    bool toggleMoonEmissive = false;
    bool toggleSphereEmissive = false;
    bool toggleConeEmissive = false;
    bool stepMoonBackward = false;
    bool stepMoonForward = false;
    bool toggleMoonMotion = false;
    bool toggleDebugUi = false;
    bool toggleCursorCapture = false;
    bool cursorCaptured = false;
    Vec2 mouseDelta{};
};
} // namespace engine
