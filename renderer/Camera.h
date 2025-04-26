#pragma once
#include <hammock/hammock.h>

using namespace hammock;

/**
 * Simple perspective camera class for view and projection matrix computation
 */
class Camera final {
public:
    Camera(HmckVec3 position,float aspect, float32_t fov, float32_t znear = 0.01f, float32_t zfar = 100000.f, float yaw = 0.f, float pitch = 0.f, float roll = 0.f): position(position), fov(fov), znear(znear), yaw(yaw), pitch(pitch), roll(roll),
        zfar(zfar), aspect(aspect) {
    }

    HmckVec3 position;
    float32_t fov, znear, zfar, aspect;
    float32_t yaw{0.f}, pitch{0.f}, roll{0.f};

    struct FrustumDirections
    {
        HmckVec3 frustumA; // top-left dir
        HmckVec3 frustumB; // top-right dir
        HmckVec3 frustumC; // bottom-left dir
        HmckVec3 frustumD; // bottom-right dir
    };


    HmckMat4 getView() {
        // First, compute the camera's orientation vectors
        // Start with default vectors
        HmckVec3 worldUp = {0.0f, -1.0f, 0.0f};

        // Calculate forward direction based on yaw and pitch (ignoring roll for now)
        // This uses spherical coordinates to calculate the direction
        forward.X = cosf(yaw) * cosf(pitch);
        forward.Y = sinf(pitch);
        forward.Z = sinf(yaw) * cosf(pitch);
        forward = HmckNorm(forward);

        // Calculate right vector as cross product of forward and world up
        HmckVec3 right = HmckNorm(HmckCross(worldUp, forward));

        // Calculate camera up vector as cross product of right and forward
        up = HmckCross(forward, right);

        // Apply roll (rotation around forward axis) if needed
        if (roll != 0.0f) {
            // Create rotation matrix around forward axis
            HmckMat4 rollRotation = HmckRotate_RH(roll, forward);

            // Apply roll to up vector
            HmckVec4 upVec4 = {up.X, up.Y, up.Z, 0.0f};
            HmckVec4 rotatedUpVec4 = HmckMulM4V4(rollRotation, upVec4);
            up = {rotatedUpVec4.X, rotatedUpVec4.Y, rotatedUpVec4.Z};

            // Recalculate right vector to maintain orthogonality
            right = HmckCross(up, forward);
        }

        // Calculate target by adding forward to position
        target = HmckAdd(position, forward);

        // Create view matrix using LookAt function
        return HmckLookAt_RH(position, target, up);
    }

    HmckMat4 getProjection() const {
        //return Projection().orthographic(-50.0, 50.0, -50.0, 50.0, znear, zfar, false);

        HmckMat4 proj = HmckPerspective_RH_ZO(fov, aspect, znear, zfar);
        proj[1][1] *= -1;
        return proj;
    }

    HmckVec3 forwardDirection() const {return HmckNorm(forward);}
    HmckVec3 upDirection() const {return HmckNorm(up);}
    HmckVec3 rightDirection() const {return HmckCross(upDirection(), forwardDirection());}

    FrustumDirections getFrustumDirections() {
        // Ensure the camera's basis vectors (forward, up) are computed.
        // Calling getView() here updates forward, up, and target.
        getView();

        // Compute right vector from current up and forward directions.
        HmckVec3 right = rightDirection();

        // Compute half-height of the near plane based on fov.
        float halfHeight = tanf(fov * 0.5f) * znear;
        // Compute half-width using the aspect ratio.
        float halfWidth = halfHeight * aspect;

        // Compute the center of the near plane.
        HmckVec3 nearCenter = HmckAdd(position, HmckMul(forward, znear));

        // Calculate direction vectors from the camera position to each corner of the near plane.
        HmckVec3 topLeftDir = HmckSub(HmckAdd(nearCenter, HmckAdd(HmckMul(up, halfHeight), HmckMul(right, -halfWidth))), position);
        HmckVec3 topRightDir = HmckSub(HmckAdd(nearCenter, HmckAdd(HmckMul(up, halfHeight), HmckMul(right, halfWidth))), position);
        HmckVec3 bottomLeftDir = HmckSub(HmckAdd(nearCenter, HmckAdd(HmckMul(up, -halfHeight), HmckMul(right, -halfWidth))), position);
        HmckVec3 bottomRightDir = HmckSub(HmckAdd(nearCenter, HmckAdd(HmckMul(up, -halfHeight), HmckMul(right, halfWidth))), position);

        // Normalize the corner directions.
        FrustumDirections frustum;
        frustum.frustumA = HmckNorm(topLeftDir);    // top-left direction
        frustum.frustumB = HmckNorm(topRightDir);   // top-right direction
        frustum.frustumC = HmckNorm(bottomLeftDir); // bottom-left direction
        frustum.frustumD = HmckNorm(bottomRightDir); // bottom-right direction

        return frustum;
    }

private:
    HmckVec3 forward;
    HmckVec3 up;
    HmckVec3 target;
};
