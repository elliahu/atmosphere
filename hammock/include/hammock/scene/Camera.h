#pragma once

#include "hammock/core/HandmadeMath.h"

namespace hammock {
    class Projection {
    public:
        HmckVec3 upNegY() { return HmckVec3{0.f, -1.f, 0.f}; }
        HmckVec3 upPosY() { return HmckVec3{0.f, 1.f, 0.f}; }

        HmckMat4 perspective(float fovy, float aspect, float zNear, float zFar, bool flip = true);

        HmckMat4 orthographic(float left, float right, float bottom, float top, float znear, float zfar, bool flip = true);

        HmckMat4 view(HmckVec3 eye, HmckVec3 target, HmckVec3 up);

        HmckMat4 inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up);
    };
}
