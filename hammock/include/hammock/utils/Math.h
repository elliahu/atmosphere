#pragma once


#include <cassert>
#include <fstream>
#include <string>
#include <random>
#include <cmath>
#include "hammock/core/HandmadeMath.h"

namespace hammock {
    namespace Math {
        inline float lerp(float a, float b, float f) {
            return a + f * (b - a);
        }

        inline HmckMat4 normal(HmckMat4 &model) {
            return HmckTranspose(HmckInvGeneral(model));
        }


        inline uint32_t padSizeToMinAlignment(uint32_t originalSize, uint32_t minAlignment) {
            return (originalSize + minAlignment - 1) & ~(minAlignment - 1);
        }

        /**
         * Calculates the orbital position around a center point.
         *
         * @param center      The center point of the orbit (Vec3).
         * @param radius      The radius of the orbit (float).
         * @param azimuth     The azimuth angle in radians (horizontal rotation, float).
         * @param elevation   The elevation angle in radians (vertical rotation, float).
         * @return            The calculated orbital position (Vec3).
         */
        inline HmckVec3 orbitalPosition(const HmckVec3 &center, float radius, float azimuth, float elevation) {
            // Calculate the position in spherical coordinates
            float x = radius * cos(elevation) * cos(azimuth);
            float y = radius * sin(elevation);
            float z = radius * cos(elevation) * sin(azimuth);

            // Translate the position relative to the center
            HmckVec3 orbitalPosition = center + HmckVec3{x, y, z};
            return orbitalPosition;
        }



    }
}