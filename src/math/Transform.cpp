#include "math/Transform.h"

namespace engine
{
Mat4 Transform::modelMatrix() const
{
    const Mat4 translation = makeTranslation(position);
    const Mat4 rotationX = makeRotationX(rotation.x);
    const Mat4 rotationY = makeRotationY(rotation.y);
    const Mat4 rotationZ = makeRotationZ(rotation.z);
    const Mat4 scaleMatrix = makeScale(scale);

    return translation * rotationZ * rotationY * rotationX * scaleMatrix;
}
} // namespace engine
