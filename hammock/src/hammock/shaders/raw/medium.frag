#version 450

#include "common/cloudsCommon.glsl"

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform Buffer {
    mat4 view;
    mat4 proj;
    vec4 eye;
    vec4 lightPosition;
    vec4 lightColor;
    float resX;
    float resY;
    float elapsedTime;
};

layout (binding = 1) uniform sampler3D sdfSampler;
layout (binding = 2) uniform sampler3D densityNoiseSampler;
layout (binding = 3) uniform sampler2D curlSampler;
layout (binding = 4) uniform sampler2D blueNoise;

layout (push_constant) uniform PushConstants {
    vec4 scattering;
    vec4 absorption;
    float g;
    float densityMultiplier;
    float densityScale;
    int enableJitter;
    float jitterStrenght;
    int lightSteps;
    float lightStepSize;
};

#define BACKGROUND vec3(0.15)

#define NUM_STEPS 128
#define NUM_LIGHT_STEPS lightSteps
#define LIGHT_STEP_SIZE lightStepSize

#define AABB_MIN vec3(- 0.5)
#define AABB_MAX vec3(0.5)

#define LIGHT_COLOR lightColor.rgb

// Scattering parameters
#define SCATTERING scattering.xyz
#define ABSORPTION absorption.xyz
#define PHASE_G g

// Convert world position to AABB UVW
vec3 worldToAABB(vec3 worldPos) {
    return (worldPos - AABB_MIN) / (AABB_MAX - AABB_MIN);
}

// Compute ray direction from camera
vec3 getRayDirection() {
    mat4 inverseView = inverse(view);
    float aspectRatio = resX / resY;
    vec2 screenUV = gl_FragCoord.xy / vec2(resX, resY) - 0.5;
    screenUV.x *= aspectRatio;
    screenUV.y *= -1.0;
    vec3 rayDir = normalize(vec3(screenUV, -1.0));
    return (inverseView * vec4(rayDir, 0.0)).xyz;
}

// Sample the signed distance field
float sdf(vec3 p) {
    vec3 uvw = worldToAABB(p);
    uvw = clamp(uvw, vec3(0.001), vec3(0.999));
    return texture(sdfSampler, uvw).r;
}

// Density sampling function
float sampleDensity(vec3 p) {
    float dist = sdf(p);
    if (dist > 0.0) return 0.0;

    vec3 uvw = worldToAABB(p) * densityScale;
    vec4 density = texture(densityNoiseSampler, uvw);
    //return (density.r + 0.5) * densityMultiplier;
    float fbm = dot(density.gba, vec3(0.625, 0.25, 0.125));
    return remap(density.r, -(1.0 - fbm), 1.0, 0.0, 1.0) * densityMultiplier;
}

vec3 lightRayAttenuation(vec3 p){
    vec3 dirToLight = normalize(lightPosition.xyz);
    vec3 attenuationAlongLightRay = vec3(1.0);

    // Lightmarch
    for(int l = 0; l < NUM_LIGHT_STEPS; l++){
        vec3 pos = p + dirToLight * l * LIGHT_STEP_SIZE;
        float density = sampleDensity(pos);
        if(density > 0.0){
            vec3 attenuation = exp(-density * LIGHT_STEP_SIZE * (SCATTERING + ABSORPTION));
            attenuationAlongLightRay *= attenuation;
        }

        if (max(attenuationAlongLightRay.r, max(attenuationAlongLightRay.g, attenuationAlongLightRay.b)) < 0.01) break;
    }

    return attenuationAlongLightRay;
}



void main() {
    vec3 rayOrigin = eye.xyz;
    vec3 rayDirection = getRayDirection();

    vec2 intersection = intersectRayAABB(rayOrigin, rayDirection, AABB_MIN, AABB_MAX);
    if (intersection.y < 0.0) {
        outColor = vec4(BACKGROUND, 1.0);
        return;
    }


    float stepSize = (intersection.y - intersection.x) / float(NUM_STEPS);
    // Get a blue noise sample; ideally, normalize your coordinates
    vec3 noise = (enableJitter == 1)? texture(blueNoise, gl_FragCoord.xy / 128.0).rgb * jitterStrenght : vec3(0.0);
    // Use one channel (e.g., the red channel) to determine an offset within one step interval
    float jitterOffset = noise.r * stepSize;
    // Apply the jitter to the start position
    vec3 p = rayOrigin + rayDirection * (intersection.x + jitterOffset);
    // Acumulated values
    vec3 totalTransmittance = vec3(1.0);
    vec3 inScattering = vec3(0.0);

    // Raymarch
    for (int i = 0; i < NUM_STEPS; i++) {
        float density = sampleDensity(p);

        if (density > 0.0) {
            // Compute the light contribution along the light ray from this point
            vec3 attenuationAlongLightRay = lightRayAttenuation(p);
            vec3 lightContribution = LIGHT_COLOR * attenuationAlongLightRay;

            // Calculate phase using the angle between the light and view direction.
            vec3 L = normalize(lightPosition.xyz - p);  // direction from point p to the light
            vec3 V = normalize(rayDirection);          // direction from point p to the camera
            float cosTheta = dot(L, V);
            float phase = henyeyGreenstein(cosTheta, PHASE_G);

            // Compute the differential in-scattering contribution using the current transmittance
            vec3 dL = totalTransmittance * density * SCATTERING * phase * lightContribution  * stepSize;
            inScattering += dL;

            // Now update total transmittance for extinction along the ray segment
            vec3 attenuation = exp(-density* stepSize* (SCATTERING + ABSORPTION));
            totalTransmittance *= attenuation;
        }

        if (max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b)) < 0.01)
        break;

        p += rayDirection * stepSize;
    }

    // Optionally, compute alpha from the accumulated optical depth,
    // but here we base it on the remaining transmittance.
    float alpha = 1.0 - max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b));

    // Use the in-scattered radiance as the final color output.
    vec3 color = inScattering;

    outColor = vec4(color, alpha);

    if(alpha == 0.0){
        outColor = vec4(BACKGROUND, 1.0);
        return;
    }
}
