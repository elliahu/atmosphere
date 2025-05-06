import toolbox;


// Input and output structures
struct VertexOutput {
    float2 uv;
    float4 position : SV_Position; // Screen-space position
};

struct PixelOutput {
    float4 color : SV_Target;
};

[vk::binding(0,0)] ConstantBuffer<CompositionBuffer> data;
[vk::binding(0, 1)] Sampler2D<float4> skyView;
[vk::binding(1, 1)] Sampler2D<float4> transmittanceLUT;


// Fullscreen vertex shader: Outputs a unit square (quad) in normalized device coordinates
[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

// Pseudo random post process that hides ugly banding and makes the result look natural
float3 postProcessColor(float2 seed, float3 input)
{
    float rand = frac(sin(dot(seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(pow(input, 1));
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);
    return input / 255;
}

// Pixel shader that up-samples currently visible part of the skyViewLUT and draws a basic sun disk
// the sun disk itself is not phyiscally accurate in terms fo its size (too big) and color
// it is here just for presentation and to actually see where the sun is (quite important)
[shader("pixel")]
PixelOutput pixelMain(VertexOutput input, float4 fragCoord : SV_Position){
    // Prep the output
    PixelOutput output;

    float2 ndc = (input.position.xy / float2(data.resX, data.resY)) * 2.0 - 1.0;
    float4 rayClip = float4(ndc, 1.0, 1.0);
    float4 rayEye = mul(data.inverseProjection, rayClip);
    rayEye.w = 0.0; // Direction vector
    float3 dir = normalize(mul(data.inverseView, rayEye).xyz);

    float phi = atan2(dir.z, dir.x);
    float u = (phi + PI) / (2 * PI);
    
    float theta = asin(dir.y);
    float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));

   float3 skyColor = skyView.SampleLevel(float2(u, v), 0.0).rgb;
    
    // Draw sun disk
    float3 sunDir = normalize(data.sunDirection.xyz);
    float sunDot = dot(dir, sunDir);
    float sunSize = 0.998; // Controls the size of the sun disk (higher = smaller)
    
    // Add the sun if we're looking in its direction
    if (sunDot > sunSize) {
        // Calculate a smooth edge for the sun
        float sunEdge = smoothstep(sunSize, sunSize + 0.001, sunDot);
        
        // Sun color 
        float3 sunColor = float3(2.0);

        // Get sun transmittance
        float3 sunTransmittance = getTransmittance(transmittanceLUT, data.cameraPosition.y * WorldScale, asin(data.sunDirection.y));

        // Apply the transmittance to the sun color
        sunColor *= sunTransmittance;
        
        // Blend the sun with the sky based on the edge factor
        //skyColor = lerp(skyColor, sunColor, sunEdge);
    }

    output.color = float4(postProcessColor(input.uv, skyColor), 1.0);

    return output;
}