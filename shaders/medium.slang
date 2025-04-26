import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

struct UniformBuffer{
    float4x4 view;
    float4x4 proj;
    float4 eye;
    float4 lightPosition;
    float4 lightColor;
    float resX;
    float resY;
    float elapsedTime;
};

struct PushConstants {
    float4 scattering;
    float4 absorption;
    float g;
    float densityMultiplier;
    float densityScale;
    int enableJitter;
    float jitterStrenght;
    int lightSteps;
    float lightStepSize;
};

[vk::push_constant] PushConstants constants;
[vk::binding(0)] ConstantBuffer<UniformBuffer> buffer;
[vk::binding(1)] Sampler3D sdfTexture;
[vk::binding(2)] Sampler3D densityNoiseTexture;
[vk::binding(3)] Sampler2D curlTexture; // TODO remove, unused
[vk::binding(4)] Sampler2D blueNoiseTexture;

#define BACKGROUND float3(0.15)

#define NUM_STEPS 128
#define NUM_LIGHT_STEPS constants.lightSteps
#define LIGHT_STEP_SIZE constants.lightStepSize

#define AABB_MIN float3(- 0.5)
#define AABB_MAX float3(0.5)

#define LIGHT_COLOR buffer.lightColor.rgb

// Scattering parameters
#define SCATTERING constants.scattering.xyz
#define ABSORPTION constants.absorption.xyz
#define PHASE_G constants.g


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

// Convert world position to AABB UVW
float3 worldToAABB(float3 worldPos) {
    return (worldPos - AABB_MIN) / (AABB_MAX - AABB_MIN);
}

// sdf if a sphere
float sdfSphere(float3 p, float radius) {
    return length(p) - radius;
}

// Sample the signed distance field
float sdf(float3 p) {
    float3 uvw = worldToAABB(p);
    uvw = clamp(uvw, float3(0.001), float3(0.999));
    return sdfTexture.SampleLevel(uvw, 0.0).r;
}

// Density sampling function
float sampleDensity(float3 p) {
    float dist = sdf(p);
    if (dist > 0.0) return 0.0;

    float3 uvw = worldToAABB(p) * constants.densityScale;
    float4 density = densityNoiseTexture.SampleLevel(uvw, 0.0);
    float fbm = dot(density.gba, float3(0.625, 0.25, 0.125));
    return remap(density.r, -(1.0 - fbm), 1.0, 0.0, 1.0) * constants.densityMultiplier;
}

// Light ray attenuation computation
float3 lightRayAttenuation(float3 p){
    float3 dirToLight = normalize(buffer.lightPosition.xyz);
    float3 attenuationAlongLightRay = float3(1.0);

    // Lightmarch
    for(int l = 0; l < NUM_LIGHT_STEPS; l++){
        float3 pos = p + dirToLight * l * LIGHT_STEP_SIZE;
        float density = sampleDensity(pos);
        if(density > 0.0){
            float3 attenuation = exp(-density * LIGHT_STEP_SIZE * (SCATTERING + ABSORPTION));
            attenuationAlongLightRay *= attenuation;
        }

        if (max(attenuationAlongLightRay.r, max(attenuationAlongLightRay.g, attenuationAlongLightRay.b)) < 0.01) break;
    }

    return attenuationAlongLightRay;
}

float2 intersectRaySphere(float3 rayOrigin, float3 rayDir, float3 sphereCenter, float sphereRadius) {
    float3 oc = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) return float2(-1.0, -1.0);
    float sqrtDisc = sqrt(discriminant);
    float t0 = (-b - sqrtDisc) / (2.0 * a);
    float t1 = (-b + sqrtDisc) / (2.0 * a);
    return float2(t0, t1);
}

// This is not optimized what so ever and is only here to present different components of volumetric rendering
// Performance will not be great but the quality will be high
// You can notice that all of the LUT computations and cloud computation use basically the same approach to some extent
[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    PixelOutput output;

    float2 ndc = (input.position.xy / float2(buffer.resX, buffer.resY)) * 2.0 - 1.0;
    float4 rayClip = float4(ndc, 1.0, 1.0);
    float4 rayEye = mul(inverse(buffer.proj), rayClip);
    rayEye.w = 0.0;
    float3 rayDirection = normalize(mul(inverse(buffer.view), rayEye).xyz);
    float3 rayOrigin = buffer.eye.xyz;

    float2 intersection = intersectRayAABB(rayOrigin, rayDirection, AABB_MIN, AABB_MAX);
    float lightRadius = 0.04;
    float2 lightHit = intersectRaySphere(rayOrigin, rayDirection, buffer.lightPosition.xyz, lightRadius);

    bool hitAABB = intersection.y >= 0.0;
    bool hitLight = lightHit.x > 0.0;

    // If the ray hits neither, draw background
    if (!hitAABB && !hitLight) {
        output.color = float4(BACKGROUND, 1.0);
        return output;
    }

    // If we only hit the light (not the AABB), draw the light
    if (!hitAABB && hitLight) {
        output.color = float4(LIGHT_COLOR, 1.0);
        return output;
    }

    // If we hit both, determine what to render based on intersection distances
    float tCloud = intersection.x;
    float tLight = hitLight ? lightHit.x : 1e30;

    // Check if there's actual cloud density between camera and light
    bool cloudBeforeLight = false;
    if (hitLight && hitAABB && tLight > tCloud) {
        float3 p = rayOrigin + rayDirection * tCloud;
        float stepSize = (min(tLight, intersection.y) - tCloud) / 8.0;
        
        for (int i = 0; i < 8; i++) {
            if (sampleDensity(p) > 0.0) {
                cloudBeforeLight = true;
                break;
            }
            p += rayDirection * stepSize;
        }
    }

    // If we hit the light and there's no cloud density before it, draw the light
    if (hitLight && (!hitAABB || !cloudBeforeLight || tLight < tCloud)) {
        output.color = float4(LIGHT_COLOR, 1.0);
        return output;
    }

    // Ray-march
    float stepSize = (intersection.y - intersection.x) / float(NUM_STEPS);
    float3 noise = (constants.enableJitter == 1) ? blueNoiseTexture.SampleLevel(input.uv * 10, 0.0).rgb * constants.jitterStrenght : float3(0.0);
    float jitterOffset = noise.r * stepSize;
    float3 p = rayOrigin + rayDirection * (intersection.x + jitterOffset);
    float3 totalTransmittance = float3(1.0);
    float3 inScattering = float3(0.0);

    for (int i = 0; i < NUM_STEPS; i++) {
        float density = sampleDensity(p);

        if (density > 0.0) {
            float3 attenuationAlongLightRay = lightRayAttenuation(p);
            float3 lightContribution = LIGHT_COLOR * attenuationAlongLightRay;

            float3 L = normalize(buffer.lightPosition.xyz - p);
            float3 V = normalize(rayDirection);
            float cosTheta = dot(L, V);
            float phase = henyeyGreenstein(cosTheta, PHASE_G);

            float3 dL = totalTransmittance * density * SCATTERING * phase * lightContribution  * stepSize;
            inScattering += dL;

            float3 attenuation = exp(-density* stepSize* (SCATTERING + ABSORPTION));
            totalTransmittance *= attenuation;
        }

        if (max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b)) < 0.01)
            break;

        p += rayDirection * stepSize;
    }

    float alpha = 1.0 - max(totalTransmittance.r, max(totalTransmittance.g, totalTransmittance.b));
    float3 color = inScattering;

    // Composite cloud over light if light is behind cloud (light is inside AABB interval)
    bool lightBehindCloud = hitLight && (lightHit.x > intersection.x) && (lightHit.x < intersection.y);
    if (lightBehindCloud) {
        color = color * alpha + LIGHT_COLOR * (1.0 - alpha);
        alpha = 1.0;
    }

    output.color = float4(color, alpha);

    if(alpha == 0.0){
        output.color = float4(BACKGROUND, 1.0);
    }

    return output;
}