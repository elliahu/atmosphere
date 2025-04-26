import toolbox;

// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

[vk::binding(0)] ConstantBuffer<CompositionBuffer> data;
[vk::binding(1)] Sampler2D<float4> terrainColorImage;
[vk::binding(2)] Sampler2D<float> terrainDepthImage;
[vk::binding(3)] Sampler2D<float4> cloudsColorImage;
[vk::binding(4)] Sampler2D<float4> transmittanceLUT;
[vk::binding(5)] Sampler3D<float4> aerialLUT;
[vk::binding(6)] Sampler2D<float4> sunShadow;
[vk::binding(7)] Sampler2D<float4> blueNoise;
[vk::binding(8)] Sampler2D<float4> skyColorImage;
[vk::binding(9)] Sampler2D<float4> godRaysImage;

// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
// This is a sort of hack that allows fragment shader to be run for full screen without any vertex data
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

// Reconstructs world space position based on the screen uv coord and normalized depth
float3 reconstructWorldPosition(float2 uv, float depth, float4x4 invProj, float4x4 invView, float near, float far) {
    // The depth is already in linear space, directly using the depth value.
    // Reconstruct view-space Z from the linear depth value
    float zView = near + depth * (far - near);

    // Convert UV to NDC [-1, 1] space
    float2 ndc = uv * 2.0 - 1.0;

    // Reconstruct homogeneous clip-space position (Z value from depth, W = 1)
    float4 clipPos = float4(ndc, depth, 1.0);

    // Convert from clip space to view space using the inverse projection matrix
    float4 viewPos = mul(invProj, clipPos);
    viewPos /= viewPos.w;  // Perform the perspective divide

    // Convert from view space to world space using the inverse view matrix
    float4 worldPos = mul(invView, viewPos);

    return worldPos.xyz;
}

// Computes shadow factor for given world space coord
float calculateShadow(float3 worldPos) {
    float4 lightClipPos = mul(data.shadowViewProj, float4(worldPos, 1.0));
    float3 ndc = lightClipPos.xyz / lightClipPos.w;
    float2 shadowUV = ndc.xy * 0.5 + 0.5;

    if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
        return 1.0;

    float shadowMapDepth = sunShadow.SampleLevel(shadowUV, 0.0).r;

    float fragmentDepthInLightSpace = lightClipPos.z / lightClipPos.w;

    float shadow = 0.0;
    float bias = 0.005; // Adjust bias to avoid shadow acne
    float2 texelSize = 1.0 / float2(data.resX, data.resY);
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            float depth = sunShadow.SampleLevel(shadowUV + offset, 0.0).r;
            shadow += fragmentDepthInLightSpace > depth + bias ? 0.0 : 1.0;
        }
    }
    
    return shadow / 9.0;
}

// Pseudo-random color post process to combat banding even more
float3 postProcessColor(float2 seed, float3 input)
{
    float rand = frac(sin(dot(
        seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(pow(input, 1));
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);

    return input / 255;
}

// Calculates god ray contribution for a given view direction and position
float3 calculateGodRays(float2 uv, float4 godRaysValue, float3 worldPos, float3 viewDir, float cloudAlpha, bool isTerrain) {
    // Calculate angle to sun for god ray attenuation
    float sunDot = max(0.0, dot(viewDir, data.sunDirection.xyz));
    
    // Different attenuation based on whether we're rendering terrain or sky
    float godRayFalloff = isTerrain ? pow(sunDot, 2.0) : pow(sunDot, 4.0);
    
    // For sky: attenuate by cloud coverage to make rays appear behind clouds
    // For terrain: use full strength
    float godRayMask = isTerrain ? 1.0 : (1.0 - pow(cloudAlpha, 0.5));
    
    // Adjust intensity based on whether we're rendering terrain or sky
    float godRayIntensity = godRaysValue.a * godRayFalloff * godRayMask * (isTerrain ? 0.5 : 0.3);
    
    if (isTerrain) {
        // For terrain, use additive blending (simpler)
        return godRaysValue.rgb * godRayIntensity;
    } else {
        // For sky, use multiplicative blending to preserve colors better
        // This creates the effect of light scattering through atmosphere
        return godRaysValue.rgb * godRayIntensity;
    }
}

// Standard soft-light blend calculation
float3 blendSoftLight(float3 base, float3 blend, float3 opacity) {
    float3 result;
    for (int i = 0; i < 3; i++) {
        result[i] = blend[i] < 0.5 
            ? base[i] - (1 - 2 * blend[i]) * base[i] * (1 - base[i])
            : base[i] + (2 * blend[i] - 1) * (sqrt(base[i]) - base[i]);
    }
    return lerp(base, result, opacity);
}

// Apply god rays differently based on the render target type
float3 applyGodRays(float3 baseColor, float3 godRayContribution, bool isTerrain) {
    if (isTerrain) {
        // Simple additive for terrain
        return baseColor + godRayContribution;
    } else {
        // Multiplicative blend for sky to preserve colors
        // I found out that it actually looks better using additive blend for clouds rather than multiplicative
        // It does crate a halo on the sky but I find that quite nice so i kept it like this
        // If you prefer standard approach described in the thesis, comment this line and uncomment the bellow one
        return baseColor + godRayContribution;
        //return blendSoftLight(baseColor, godRayContribution, godRayContribution);
    }
}

// Reconstructs terrain color
float4 terrainColor(float2 uv, float4 godRaysValue) {
    // Sample the terrain
    float4 terrainColor = terrainColorImage.SampleLevel(float2(uv.x, uv.y), 0.0);
    float3 color = terrainColor.rgb;

    if (terrainColor.a == 1.0){
        // Sample the depth buffer
        float depth = terrainDepthImage.SampleLevel(float2(uv.x, uv.y), 0.0).r;

        // Compute terrain world position
        float3 terrainWorldPos = reconstructWorldPosition(uv, depth, data.inverseProjection, data.inverseView, 0.1, 300.0);

        // Compute terrain altitude (height above ground)
        float terrainAltitude = terrainWorldPos.y * WorldScale;

        // Calculate view direction
        float3 viewDir = normalize(terrainWorldPos - data.cameraPosition.xyz);
        
        // Get god ray contribution and apply it
        if(data.applyGodRays == 1){
            float3 godRayContribution = calculateGodRays(uv, godRaysValue, terrainWorldPos, viewDir, 0.0, true);
            color = applyGodRays(color, godRayContribution, true);
        }

        // Sample the transmittance LUT
        float4 ap = aerialLUT.SampleLevel(float3(uv, 1.0 - depth), 0.0);
        float3 inScatter = ap.xyz; // Atmospheric in-scattering
        float eyeTransmittance = ap.w;
        float3 sunTransmittance = getTransmittance(transmittanceLUT, terrainAltitude, asin(data.sunDirection.y));

        float shadowFactor = 1.0;// calculateShadow(terrainWorldPos);

        color *= (max(0.25, shadowFactor)) * sunTransmittance * eyeTransmittance;
        color += inScatter;
    }

    return float4(postProcessColor(uv, color), terrainColor.a);
}

// Reconstructs cloud color
float3 skyColor(float2 uv, float4 godRaysValue){
    // Sample cloud color
    float4 cloudColor = cloudsColorImage.SampleLevel(uv, 0.0);
    float3 directLight = cloudColor.r * data.sunColor.rgb;
    float3 ambientLight = cloudColor.g * data.ambientColor.rgb;
    float3 cloudIntensity = directLight + ambientLight;
    float cloudAlpha = cloudColor.a;
    float cloudDepth = cloudColor.b;

    // Sample sky color
    float4 skyColor = skyColorImage.SampleLevel(uv, 0.0);
    float3 color = cloudIntensity;
    
    // Reconstruct world position and view direction
    float3 worldPos = reconstructWorldPosition(uv, cloudDepth, data.inverseProjection, data.inverseView, 0.1, 300.0);
    float3 viewDir = normalize(worldPos - data.cameraPosition.xyz);    

    // Compute the altitude of the clouds
    float cloudAltitude = worldPos.y * WorldScale;

    // Sample the transmittance LUT
    float4 ap = aerialLUT.SampleLevel(float3(uv, 1.0 - cloudDepth), 0.0);
    float3 inScatter = ap.xyz; // Atmospheric in-scattering
    float eyeTransmittance = ap.w;
    float3 sunTransmittance = getTransmittance(transmittanceLUT, cloudAltitude, asin(data.sunDirection.y));

    color *= sunTransmittance * eyeTransmittance;
    color += inScatter;

    // Blend clouds over the sky with god rays
    color = blend(skyColor.rgb, color, cloudAlpha);

    if (data.applyGodRays == 1) {
        float3 godRayContribution = calculateGodRays(uv, godRaysValue, worldPos, viewDir, cloudAlpha, true);
        godRayContribution *= sunTransmittance;
        color = applyGodRays(color, godRayContribution, true);
    }

    return postProcessColor(uv, color);
}

[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position) {
    PixelOutput output;

    // Normalize screen coordinates
    float2 screenUV = fragCoord.xy / float2(data.resX, data.resY);

    // Define preview size and position
    float2 previewSize = float2(0.2, 0.2); // 20% of screen
    float2 previewPos = float2(0.8, 0.0);  // Top-right corner

    bool showPreview = all(screenUV >= previewPos) && all(screenUV <= (previewPos + previewSize));
#ifndef DEBUG
    showPreview = false;
#endif

    // There is also a debug view that displays sun shadow map
    // try it out by defining DEBUG macro at the top and recompiling this shader

    if (showPreview) {
        // Map screenUV to shadow map UV
        float2 shadowUV = (screenUV - previewPos) / previewSize;
        float depth = sunShadow.SampleLevel(shadowUV, 0.0).r;
        output.color = float4(depth.xxx, 1.0); // grayscale
    } else {
        float4 godRays = saturate(godRaysImage.SampleLevel(input.uv, 0.0));

        float4 terrainColorSample = terrainColor(input.uv, godRays);
        output.color = terrainColorSample;

        if (terrainColorSample.a < 1.0) {
            output.color = float4(skyColor(input.uv, godRays), 1.0);
        }
    }

    return output;
}