#ifndef FUNCTIONS_GLSL
#define FUNCTIONS_GLSL

#include "consts.glsl"

float lightAttenuation(float d, float linearTerm, float quadraticTerm){
    return 1.0 / (1.0 + linearTerm * d + quadraticTerm * d * d);
}

// Function to calculate spherical UV coordinates from a direction vector
vec2 toSphericalUV(vec3 direction)
{
    float phi = atan(direction.z, direction.x);
    float theta = acos(direction.y);
    float u = phi / (2.0 * PI) + 0.5;
    float v = theta / PI;
    return vec2(u, v);
}

float linearDepth(float depth, float nearPlane, float farPlane)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

#endif