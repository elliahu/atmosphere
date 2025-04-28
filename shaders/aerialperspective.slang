import toolbox;

#define PER_SLICE_SAMPLES 8


[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] Sampler2D Transmittance;
[vk::binding(1,1)] Sampler2D multipleScattering;
[vk::binding(2,1)] Sampler2D shadowMap;
[vk::binding(3,1)] RWTexture3D<float4> aerialPerspective;


// Computes a ray direction based on uv coords and camera frustum vectors
float3 getRayDir(float2 uv) {
    // Bilinear interpolation between the 4 frustum corners
    float3 topDir = lerp(params.frustumD.xyz, params.frustumC.xyz, uv.x);
    float3 bottomDir = lerp(params.frustumB.xyz, params.frustumA.xyz, uv.x);
    return normalize(lerp(bottomDir, topDir, uv.y));
}


[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID)
{
    // Validate dimensions
    int width, height, depth;
    aerialPerspective.GetDimensions(width, height, depth);
    if(threadIdx.x >= width || threadIdx.y >= height) return;

    // get uv coords
    float xf = (threadIdx.x + 0.5) / width;
    float yf = (threadIdx.y + 0.5) / height;

    // compute the hight in the atmosphere
    const float AtmosEyeHeight = WorldScale *  params.Eye.y;

    // Compute ray origin and direction
    float3 ori = float3(0, AtmosEyeHeight, 0);
    float3 dir = getRayDir(float2(xf, yf));

    // cos sun dot ray
    float u = dot(normalize(params.SunDirection.xyz), dir);

    // How far
    float maxT = 0;
    if (!intersectRaySphere(ori + float3(0, PlanetRadius, 0), dir, PlanetRadius, maxT)) {
        intersectRaySphere(ori + float3(0, PlanetRadius, 0), dir, AtmosphereRadius, maxT);
    }

    float sliceDepth = MaxDistance / depth;
    float halfSliceDepth = 0.5 * sliceDepth;
    float tBeg = 0;
    float tEnd = min(halfSliceDepth, maxT);

    float3 sumSigmaT = float3(0, 0, 0);
    float3 inScatter = float3(0, 0, 0);

    // Dithering, simple but effective
    float rand = frac(sin(dot(float2(xf, yf), float2(12.9898, 78.233) * 2.0)) * 43758.5453);

    // For each slice, compute the in scattered light and transmittance
    for(int z = 0; z < depth; ++z){
        float dt = (tEnd - tBeg) / PER_SLICE_SAMPLES;
        float t = tBeg;

        for(int i = 0; i < PER_SLICE_SAMPLES; ++i){
            float nextT = t + dt;

            float  midT = lerp(t, nextT, rand);
            float3 posR = float3(0, ori.y + PlanetRadius, 0) + dir * midT;
            float  h    = length(posR) -PlanetRadius;

            float SunTheta = asin(params.SunDirection.y);

            float3 sigmaS, sigmaT;
            getSigmaST(h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

            if (!existsRaySphereIntersection(posR, params.SunDirection.xyz, PlanetRadius)){
                float3 shadowPos  = params.Eye.xyz + dir * midT / WorldScale;
                float4 shadowClip = mul(params.ShadowViewProj, float4(shadowPos, 1.0));
                float3 shadowNDC  = shadowClip.xyz / shadowClip.w;
                float2 shadowUV = shadowNDC.xy * 0.5 + 0.5;

                bool inShadow = true;
                if(all(saturate(shadowUV) == shadowUV)){
                    float rayZ = shadowClip.z;
                    float smZ = shadowMap.SampleLevel(shadowUV, 0.0).r;
                    inShadow = rayZ >= smZ;
                }

                if(!inShadow){
                    float3 rho = evalPhaseFunction(h, u);
                    float3 sunTrans = getTransmittance(Transmittance, h, SunTheta);
                    inScatter += dt * eyeTrans * sigmaS * rho * sunTrans;
                }
            }

            float tx = h / (AtmosphereRadius - PlanetRadius);
            float ty = 0.5 + 0.5 * sin(SunTheta);
            float3 ms = multipleScattering.SampleLevel( float2(tx, ty), 0).rgb;
            inScatter += dt * eyeTrans * sigmaS * ms;
            
            sumSigmaT += deltaSumSigmaT;
            t = nextT;
        }

        float transmittance = relativeLuminance(exp(-sumSigmaT));
        aerialPerspective[int3(threadIdx.xy, z)] = float4(inScatter, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}