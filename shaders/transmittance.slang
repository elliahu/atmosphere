import toolbox;

// This value can be lowered for in production to around 40 without any visible difference
#define STEP_COUNT 128 

[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] RWTexture2D<float4> transmittance;

// Performs basic raymarching to evaluate the transmittance
[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID){
    int width, height;
    transmittance.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    // Indexed by height and sun theta
    float theta = asin(lerp(-1.0, 1.0, float(threadIdx.y + .5) / float(height)));
    float h = lerp(0.0, AtmosphereRadius - PlanetRadius, float(threadIdx.x + .5) / float(width));

    float2 o = float2(0, PlanetRadius + h);
    float2 d = float2(cos(theta), sin(theta));

    // Find the intersection with the planet or atmosphere
    float t = 0;
    if (!intersectRayCircle(o, d, PlanetRadius, t)) {
        intersectRayCircle(o, d, AtmosphereRadius, t);
    }
        
    // Compute the transmittance for all theta and h
    float2 end = o + t * d;
    float3 sum = 0.;
    for(int i = 0; i < STEP_COUNT; ++i){
        float2 pi = lerp(o, end, float(i) / float(STEP_COUNT));
        float hi = length(pi) - PlanetRadius;
        float3 sigma = getSigmaT(hi);
        sum += sigma;
    }

    float3 result = exp(-sum * (t / float(STEP_COUNT)));
    transmittance[threadIdx.xy] = float4(result, 1);
}