import toolbox;


#define PLANET_ALBEDO float3(0.3)
#define DIR_SAMPLE_COUNT 32
#define RAYMARCH_STEP_COUNT 20 

[vk::binding(0,0)] ConstantBuffer<AtmosphereParams> params;
[vk::binding(0,1)] Sampler2D Transmittance;
[vk::binding(1,1)] RWTexture2D<float4> MultiScattering;

// Computes the radical inverse using base 2.
float radicalInverse_VdC(uint bits){
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
    bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
    bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);
    return float(bits) * 2.3283064365386963e-10; // Division by 2^32.
}

// Returns a 2D Hammersley sample in [0,1]^2.
float2 hammersley(uint i, uint N){
    return float2(float(i) / float(N), radicalInverse_VdC(i));
}

// Maps a 2D point from the Hammersley sequence to a uniform direction on the sphere.
float3 uniformSampleSphere(uint i, uint N){
    float2 xi = hammersley(i, N);
    // Map xi.x to [0,1] -> theta in [0, PI] using the inverse CDF of the cosine law.
    float theta = acos(1.0 - 2.0 * xi.x);
    // Map xi.y to phi in [0, 2PI].
    float phi = 2.0 * 3.14159265 * xi.y;
    float sinTheta = sin(theta);
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cos(theta));
}

// Analytical integration as proposed by Sébastien Hillaire (or rather based on his proposal)
void integrate(float3 worldOri, float3 worldDir, float sunTheta, float3 toSunDir, out float3 innerL2, out float3 innerF){
    // cos sun dot ray
    float u = dot(worldDir, toSunDir);

    // Find the intersection with the planet and atmosphere spheres
    float endT = 0;
    bool groundInct = intersectRaySphere(worldOri, worldDir, PlanetRadius, endT);
    if (!groundInct) {
        intersectRaySphere(worldOri, worldDir, AtmosphereRadius, endT);
    }

    float dt = endT / RAYMARCH_STEP_COUNT;
    float halfDt = 0.5 * dt;
    float t = 0;

    float3 sumSigmaT = float3(0, 0, 0);

    float3 sumL2 = float3(0, 0, 0);
    float3 sumF = float3(0, 0, 0);
    for(int i = 0; i < RAYMARCH_STEP_COUNT; ++i){
        float midT = t + halfDt;
        t += dt;

        float3 worldPos = worldOri + midT * worldDir;
        float h = length(worldPos) - PlanetRadius;

        float3 sigmaS, sigmaT;
        getSigmaST(h, sigmaS, sigmaT);

        float3 deltaSumSigmaT = dt * sigmaT;
        float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

        if (!existsRaySphereIntersection(worldPos, toSunDir, PlanetRadius)){
            float3 rho = evalPhaseFunction(h, u);
            float3 sunTransmittance = getTransmittance(Transmittance, h, sunTheta);

            // Second order scattering
            sumL2 += dt * transmittance * sunTransmittance * sigmaS * rho * SunIntensity;
        }

        sumF += dt * transmittance * sigmaS;
        sumSigmaT += deltaSumSigmaT;
    }

    if(groundInct){
        float3 transmittance = exp(-sumSigmaT);
        float3 sunTransmittance = getTransmittance(Transmittance, 0, sunTheta);
        sumL2 += transmittance * sunTransmittance * max(0, toSunDir.y) * SunIntensity * (PLANET_ALBEDO / PI);
    }

    innerL2 = sumL2;
    innerF  = sumF;
}

// Wrapper so that the compute shader entry function is not total mess
float3 computeM(float h, float sunTheta){
    float3 worldOri = { 0, h + PlanetRadius, 0 };
    float3 toSunDir = { cos(sunTheta), sin(sunTheta), 0 };

    // Uniformly sample the unit sphere and compute 2nd order scattering
    float3 sumL2 = float3(0, 0, 0);
    float3 sumF = float3(0, 0, 0);
    for(int i = 0; i < DIR_SAMPLE_COUNT; ++i){
        float3 worldDir = uniformSampleSphere(i, DIR_SAMPLE_COUNT);

        float3 innerL2, innerF;
        integrate(worldOri, worldDir, sunTheta, toSunDir, innerL2, innerF);

        sumL2 += innerL2;
        sumF += innerF;
    }

    float3 l2 = sumL2 / DIR_SAMPLE_COUNT;
    float3 f  = sumF  / DIR_SAMPLE_COUNT;
    return l2 / (1 - f);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    MultiScattering.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height) return;

    // Indexed by height and sun theta
    float sinSunTheta = lerp(-1, 1, (threadIdx.y + 0.5) / height);
    float sunTheta = asin(sinSunTheta);
    float h = lerp(0.0, AtmosphereRadius - PlanetRadius, (threadIdx.x + 0.5) / width);

    MultiScattering[threadIdx.xy] = float4(computeM(h, sunTheta), 1);
}