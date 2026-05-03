#pragma once

#include "geometry/Geometry.h"

namespace engine
{
Geometry makePlaneGeometry();
Geometry makeCubeGeometry();
Geometry makeCylinderGeometry(unsigned int segmentCount = 24);
Geometry makePyramidGeometry();
Geometry makeSphereGeometry(unsigned int stackCount = 16, unsigned int sliceCount = 24);
Geometry makeConeGeometry(unsigned int segmentCount = 24);
Geometry makeCapsuleGeometry(unsigned int hemisphereSegments = 12, unsigned int ringSegments = 24);
Geometry makeTorusGeometry(unsigned int majorSegments = 24, unsigned int minorSegments = 16);
} // namespace engine
