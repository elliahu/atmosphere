#pragma once
#include <hammock/hammock.h>

#define CWD(path) "../" path
#define ASSET_PATH(asset) CWD("assets/" asset)
#define COMPILED_SHADER_PATH(shader) CWD("spv/" shader ".spv")
#define GROUPS_COUNT(res,size) ((res+size-1)/size)

enum class WeatherMap{
    Stratus,
    Stratocumulus,
    Cumulus,
    Nubis,
};

enum class TerrainType{
    Default,
    Mountain,
};

struct GeometryPushConstantData {
    HmckMat4 modelViewProjection;
    HmckVec4 lightDirection;
    HmckVec4 lightColor;
    HmckVec4 ambientLightColor = {0.1f, 0.1f,0.1f,0.0f};
};

// Data for cloud pass passed as uniform buffer
struct CloudsUniformBufferData {
    HmckMat4 invView;
    HmckMat4 invProj;
    HmckMat4 view;
    HmckMat4 proj;
    HmckVec4 cameraPosition;
    HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 5.0f}; // W is strength
    HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
    HmckVec4 skyColorZenith{59.0 / 255.0, 110.0 / 255.0, 219.0 / 255.0};
    HmckVec4 windDirection{};
    float resX;
    float resY;
    float fov;
    float znear;
    float zfar;
    float time = 0.0f;
    int frameIndexMod16;
    int _padding[2];
};

// Data for cloud pass passed as push constant block
struct CloudsPushConstantData {
    float anvilBias = 0.0f;
    float globalDensity = 0.3f;
    float globalCoverage = 0.0;
    float baseMultiplier = 0.8f;
    float detailMultiplier = 0.75;
    float cloudSpeed = 840.f;
    float baseScale = 50.f;
    float detailScale = 50.0f;
    float curliness = 2.0f;
    float absorption = 0.0042f;
    float eccentricity = 0.35f;
    float intensity = .95f;
    float spread = 1.0f;
    float ambientStrength = 0.25;
    int DEBUG_epicView = 0;
    int DEBUG_cheapSampleDistance = 93000;

#if defined(HIGH_QUALITY_CLOUDS)
    int DEBUG_maxSamples = 256;
    int DEBUG_maxLightSamples = 10;
#else
    int DEBUG_maxSamples = 128;
    int DEBUG_maxLightSamples = 4;
#endif

    int DEBUG_earlyTermination = 0;
    int DEBUG_lateTermination = 0;

#if defined(HIGH_QUALITY_CLOUDS)
    float DEBUG_longStepMulti = 3.f;
#else
    float DEBUG_longStepMulti = 2.5f;
#endif
};

// Atmospheric pass data
struct AtmosphereUniformBufferData {
    HmckMat4 inverseProjection;
    HmckMat4 inverseView;
    HmckMat4 shadowViewProj;
    HmckVec4 eye;
    HmckVec4 sunDirection;
    HmckVec4 frustumA, frustumB, frustumC, frustumD;
    float resX;
    float resY;
};

// Data for post process pass passed as push constant block
struct PostProcessingPushConstantData {
    HmckVec4 colorTint{1.0f, 1.0f, 1.0f, 0.0f};
    float contrast = 1.0; // Default: 1.0, Range: 0.5 to 2.0
    float brightness = 0.0; // Default: 0.0, Range: -1.0 to 1.0
    float vignetteStrength = 2.f; // Default 2.0, Range: 0.0 to 3.0
    float vignetteSoftness = 1.0f; // Default: 0.5, Range: 0.0 to 2.0
    float grainAmount = 0.035f; // Default: 0.0, Range: 0.0 to 0.1
    float time = 0.0f;
};

struct GodRaysCoefficients {
    float lssposX = 1.f; // light screen space position X
    float lssposY = 1.f; // light screen space position Y
    int num_samples = 56;
    float density = 0.7;
    float exposure = 0.8;
    float decay = 0.9;
    float activeDistance = 1.0;
    float weight = 1.0;
    float alpha = 0.3;
};

struct CompositionData {
    HmckMat4 inverseView;
    HmckMat4 inverseProjection;
    HmckMat4 shadowViewProj;
    HmckVec4 sunDirection;
    HmckVec4 sunColor;
    HmckVec4 ambientColor;
    HmckVec4 cameraPosition;
    float resX;
    float resY;
    int applyGodRays = 0;
};