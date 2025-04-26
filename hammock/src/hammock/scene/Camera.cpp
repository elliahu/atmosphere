#include "hammock/scene/Camera.h"

#include <cassert>
#include <cmath>
#include <limits>

HmckMat4 hammock::Projection::perspective(float fovy, float aspect, float zNear, float zFar, bool flip ) {
    assert(HmckABS(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    HmckMat4 projection = HmckPerspective_RH_ZO(fovy, aspect, zNear, zFar);
    if (flip) {
        projection[1][1] *= -1;
    }
    return projection;
}

HmckMat4 hammock::Projection::orthographic(float left, float right, float bottom, float top, float znear, float zfar, bool flip) {
    HmckMat4 projection = HmckOrthographic_RH_ZO(left, right, bottom, top, znear, zfar);
    if (flip) {
        projection[1][1] *= -1;
    }
    return projection;
}


HmckMat4 hammock::Projection::view(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
   return HmckLookAt_RH(eye, target, up);
}

HmckMat4 hammock::Projection::inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up) {
    return HmckInvLookAt(HmckLookAt_RH(eye, target, up));
}
