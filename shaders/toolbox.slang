// General constants
static const float PI = 3.14159265358979323846;
static const float SunIntensity = 3.0;
static const float WorldScale = 500.0;
static const float MaxDistance = 10000.0; // Distance of the aerial perspective LUT

// Atmosphere parameters
static const float PlanetRadius = 6378000.0; // 6378 km
static const float AtmosphereRadius = 6460000.0; // 6460 km
static const float OzoneThickness = 30000.0; // 30 km
static const float OzoneCenterHeight = 25000.0; // 25 km
static const float HDensityMie = 1200.0;
static const float AbsorbMie = 4.4 * 1e-6;
static const float AsymmetryMie = 0.8;
static const float ScatterMie = 3.996 * 1e-6;
static const float HDensityRayleigh = 8000.0;
static const float3 AbsorbOzone = float3(0.65, 1.881, 0.085) * 1e-6;
static const float3 ScatterRayleigh = float3(5.802, 13.558, 33.1) * 1e-6;


/*
// For Mars, uncomment this and comment the above
static const float PlanetRadius = 3389500.0; // Mars radius ≈ 3389.5 km
static const float AtmosphereRadius = 3489500.0; // Thin atmosphere (~10 km thick)
static const float OzoneThickness = 0.0; // Mars has no significant ozone layer
static const float OzoneCenterHeight = 0.0; // No ozone peak

// Mie scattering (dust-heavy but low density)
static const float HDensityMie = 11000.0; // Less gravity, slower falloff than Earth
static const float AbsorbMie = 3.0 * 1e-7; // Slightly more absorbing due to dust
static const float AsymmetryMie = 0.85; // Martian dust is highly forward-scattering
static const float ScatterMie = 2.0 * 1e-7; // Very low Mie scattering

// Rayleigh scattering (mostly CO2, low density)
static const float HDensityRayleigh = 11000.0; // Extended falloff due to low pressure
static const float3 ScatterRayleigh = float3(0.0001, 0.0002, 0.0003); // Near-zero (very weak blue sky)
static const float3 AbsorbOzone = float3(0.0, 0.0, 0.0); // No ozone absorption
*/


// Types
// This is buffer that holds the data for the clouds
struct CloudData{
    float4x4 invView;
    float4x4 invProj;
    float4x4 view;
    float4x4 proj;
    float4 cameraPosition;
    float4 lightColor; // W is strength
    float4 lightDirection;
    float4 skyColorZenith;
    float4 windDirection;
    float resX;
    float resY;
    float fov;
    float near;
    float far;
    float time = 0.0f;
    int frameIndexMod16;
    int _padding[2];
};

// This is a common buffer for the atmosphere LUTs computation
struct AtmosphereParams{
    float4x4 inverseProjection;
    float4x4 inverseView;
    float4x4 ShadowViewProj;
    float4 Eye;
    float4 SunDirection;
    float4 frustumA, frustumB, frustumC, frustumD;
    float resX;
    float resY;
};

// Data for composition pass
struct CompositionBuffer{
    float4x4 inverseView;
    float4x4 inverseProjection;
    float4x4 shadowViewProj;
    float4 sunDirection;
    float4 sunColor;
    float4 ambientColor;
    float4 cameraPosition;
    float resX;
    float resY;
    int applyGodRays;
}

// Atmosphere functions

// Samples transmittance from the LUT
float3 getTransmittance(Sampler2D T, float h, float theta){
    float u = h / (AtmosphereRadius - PlanetRadius);
    float v = 0.5 + 0.5 * sin(theta);
    return T.SampleLevel(float2(u, v), 0.0).xyz;
}

// Compute sigma_st
void getSigmaST(float h, out float3 sigmaS, out float3 sigmaT){
    float3 rayleigh = ScatterRayleigh.xyz * exp(-h / HDensityRayleigh);
    float mieDensity = exp(-h / HDensityMie);
    float mieS = ScatterMie * mieDensity;
    float mieT = (ScatterMie + AbsorbMie) * mieDensity;
    float3 ozone = AbsorbOzone.xyz * max(0.0f, 1 - 0.5 * abs(h - OzoneCenterHeight) / OzoneThickness);
    sigmaS = rayleigh + mieS;
    sigmaT = rayleigh + mieT + ozone;
}

// Computes sigma_t
float3 getSigmaT(float h){
    float3 rayleigh = ScatterRayleigh.xyz * exp(-h / HDensityRayleigh);
    float mie = (ScatterMie + AbsorbMie) * exp(-h / HDensityMie);
    float3 ozone = AbsorbOzone.xyz * max(
        0.0f, 1 - 0.5 * abs(h - OzoneCenterHeight) / OzoneThickness);
    return rayleigh + mie + ozone;
}

// Evaluates phase function for Rayleigh and Mie scattering
float3 evalPhaseFunction(float h, float u){
    float3 sRayleigh = ScatterRayleigh.xyz * exp(-h / HDensityRayleigh);
    float sMie = ScatterMie * exp(-h / HDensityMie);
    float3 s = sRayleigh + sMie;
    float g = AsymmetryMie, g2 = g * g, u2 = u * u;
    float pRayleigh = 3 / (16 * PI) * (1 + u2);
    float m = 1 + g2 - 2 * g * u;
    float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));
    float3 result;
    result.x = s.x > 0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0;
    result.y = s.y > 0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0;
    result.z = s.z > 0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0;
    return result;
}

// Compute relative luminance
float relativeLuminance(float3 c){
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

// Toolbox function

// Remaps a value from one range to another
float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

// Clamps a value between 0 and 1
float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

// Powdered sugar function
float powder(float d) {
    return (1.0 - exp(-2.0 * d));
}

// Computes clip space coordinates for a fragment coordinate
float3 computeClipSpaceCoord(uint2 fragCoord, int2 res) {
    float2 rayNds = 2.0 * float2(fragCoord.xy) / float2(res) - 1.0;
    return float3(rayNds, 1.0);
}


// PHASE FUNCTIONS

// Heney-Greenstein phase function
float henyeyGreenstein(float sundotrd, float g) {
    float gg = g * g;
    return (1.0 - gg) / pow(1.0 + gg - 2.0 * g * sundotrd, 1.5);
}

// Heney-Greenstein phase function with a dual lobe used in Frostbite engine
float dualLobePhase(float sundotrd, float phaseG) {
    return lerp(henyeyGreenstein(sundotrd, -phaseG), henyeyGreenstein(sundotrd, phaseG), saturate(sundotrd * 0.5 + 0.5));
}

// Henyey-Greenstein phase function with a modified eccentricity
float henyeyGreensteinModified(float sundotrd, float ecc) {
    return ((1.0 - ecc * ecc) / pow((1.0 + ecc * ecc - 2.0 * ecc * sundotrd), 1.5)) / (4.0 * PI);
}

// Dual lobe phase function with a modified eccentricity used in Decima engine in Nubis cloud system
float directedPhase(float sundotrd, float eccentricity, float silverIntensity, float silverSpread) {
    return max(henyeyGreensteinModified(sundotrd, eccentricity), silverIntensity * henyeyGreensteinModified(sundotrd, 0.999 - silverSpread));
}

// Isotropic phase function
float isophase() {
    return 1.0 / (4.0 * PI);
}

// RAYCASTING
float2 intersectRayAABB(float3 rayOrigin, float3 rayDir, float3 aabbMin, float3 aabbMax) {
    float3 tMin = (aabbMin - rayOrigin) / rayDir;
    float3 tMax = (aabbMax - rayOrigin) / rayDir;
    float3 t1 = min(tMin, tMax);
    float3 t2 = max(tMin, tMax);
    float tEnter = max(t1.x, max(t1.y, t1.z));
    float tExit = min(t2.x, min(t2.y, t2.z));
    if (tExit < 0.0 || tEnter > tExit) return float2(-1.0);
    return float2(tEnter, tExit);
}

float2 intersectRaySphere(float3 c, float r, float3 o, float3 d) {
    float3 of = o - c;
    const float a = 1.0;
    float b = 2.0 * dot(of, d);
    float c = dot(of, of) - r * r;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0) {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0) {
            return float2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }
    return float2(0.0, 0.0);
}

bool existsRayCircleIntersection(float2 o, float2 d, float R){
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    return (delta >= 0) && ((C <= 0) | (B <= 0));
}

bool existsRaySphereIntersection(float3 o, float3 d, float R){
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    return (delta >= 0) && ((C <= 0) | (B <= 0));
}

bool intersectRayCircle(float2 o, float2 d, float R, out float t){
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if(delta < 0)
        return false;
    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}

bool intersectRaySphere(float3 o, float3 d, float R, out float t){
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if(delta < 0)
        return false;
    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}

// Blends two colors based on an alpha value
float3 blend(float3 base, float3 blend, float alpha){
    return base * (1.0 - alpha) + blend;
}

// Computes inverse of a 4x4 matrix
// For some reason, the inverse function is not available in the Slang nor HLSL standard library
// Might not give best performance, but it is only used in the participating medium playground scene where the performance is not critical
float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}
