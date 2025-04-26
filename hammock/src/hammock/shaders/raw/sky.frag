#version 450

#include "common/cloudsCommon.glsl"

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 outColor;

layout (binding = 2) uniform SunAndSkyUBO {
    vec4 lightColor;
    vec4 lightDirection;
    vec4 windDirection;
};

layout (binding = 0) uniform CameraUBO {
    mat4 inv_view;
    mat4 inv_proj;
    mat4 invViewProj;
    vec4 cameraPosition;
    float resX;
    float resY;
    float fov;
};

layout (binding = 1) uniform TimeUBO {
    float iTime;
    float timeOfDay;
};

void main() {
    // Compute ray direction in world space.
    vec4 ray_clip = vec4(computeClipSpaceCoord(uvec2(gl_FragCoord.xy), ivec2(resX, resY)), 1.0);
    vec4 ray_view = inv_proj * ray_clip;
    ray_view = vec4(ray_view.xy, -1.0, 0.0);
    vec3 rayDirection = normalize((inv_view * ray_view).xyz);

    // Base sky color.
    vec3 skyColor = vec3(59.0/255.0, 110.0/255.0, 219.0/255.0);

    // Sun disk parameters.
    // The sun's full angular diameter is 0.5°, so the half-angle is 0.25°.
    const float sunHalfAngle = 2.5 * 3.14159265 / 180.0; // in radians

    // Get the actual sun direction.
    vec3 sunDir = normalize(lightDirection.xyz);

    // Compute an effective sun direction for drawing the disk.
    // If the sun is below the horizon, project its direction onto the horizon (y=0).
    vec3 effectiveSunDir = sunDir;
    if (sunDir.y < 0.0) {
        effectiveSunDir.y = 0.0;
        effectiveSunDir = normalize(effectiveSunDir);
    }

    // Compute the angular difference between the current ray and the effective sun direction.
    float dotSun = dot(rayDirection, effectiveSunDir);
    float angleDiff = acos(clamp(dotSun, -1.0, 1.0));
    // Use smoothstep to create a soft-edged disk.
    float sunMask = 1.0 - smoothstep(sunHalfAngle * 0.9, sunHalfAngle, angleDiff);

    // Also, clip out fragments that are below the horizon.
    float horizonMask = step(0.0, rayDirection.y);
    float finalSunMask = sunMask * horizonMask;

    // Blend the sun color (lightColor.rgb) over the sky.
    vec3 finalColor = mix(skyColor, lightColor.rgb, finalSunMask);

    outColor = vec4(finalColor, 1.0);
}