#version 450

#include "common/types.glsl"
#include "common/consts.glsl"
#include "common/functions.glsl"
#include "common/global_binding.glsl"
#include "common/projection_binding.glsl"
#include "common/mesh_binding.glsl"
#include "common/composition_binding.glsl"

//inputs
layout (location = 0) in vec2 uv;

// outputs
layout (location = 0) out vec4 outColor;


// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    // Precalculate vectors and dot products
    vec3 H = normalize(V + L);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);

    // Light color fixed
    vec3 lightColor = vec3(1.0);

    vec3 color = vec3(0.0);

    if (dotNL > 0.0) {
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, roughness);
        // G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, F0);
        vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        color += (kD * albedo / PI + spec) * dotNL;
    }

    return color;
}


// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}


vec3 prefilteredReflection(vec2 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(prefilteredSampler, R, lodf).rgb;
    vec3 b = textureLod(prefilteredSampler, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

void main()
{
    vec3 N = normalize(texture(normalSampler, uv).rgb);
    vec3 position = texture(positionSampler, uv).rgb;
    vec3 V = normalize(-position);
    vec3 R = reflect(-V, N);
    R = mat3(projection.inverseView) * R;
    vec3 albedo = texture(albedoSampler, uv).rgb;
    vec4 material = texture(materialPropertySampler, uv);
    float roughness = material.r;
    float metallic = material.g;
    float ao = material.b;
    float alphaMode = material.a;
    vec3 accumulatedColor = texture(accumulationSampler, uv).rgb;
    float transparencyWeight = texture(transparencyWeightSampler, uv).r;

    float exposure = projection.exposureGammaWhitePoint.x;
    float gamma = projection.exposureGammaWhitePoint.y;
    float whitePoint = projection.exposureGammaWhitePoint.z;

    if (transparencyWeight > 0.0) {
        albedo = (albedo * (1.0 - transparencyWeight)) + (accumulatedColor * transparencyWeight);
    }

    if (material == vec4(-1.0)) // Background pixels are skipped
    {
        outColor = vec4(albedo, 1.0);
        return;
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);


    vec2 brdf = texture(brdfLUTSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefilteredReflection(toSphericalUV(R), roughness).rgb;
    vec3 irradiance = texture(irradinaceSampler, toSphericalUV(mat3(projection.inverseView) * N)).rgb;

    // Diffuse based on irradiance
    vec3 diffuse = irradiance * albedo;

    vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

    // Specular reflectance
    vec3 specular = reflection * (F * brdf.x + brdf.y);

    // Ambient part
    vec3 kD = 1.0 - F;
    kD *= 1.0 - metallic;
    vec3 ambient = (kD * diffuse + specular) * vec3(ao);

    vec3 color = ambient * ao;



    // Tone mapping
    color = Uncharted2Tonemap(color * exposure);
    color = color * (1.0f / Uncharted2Tonemap(vec3(whitePoint)));

    // Gamma correction
    color = pow(color, vec3(1.0f / gamma));

    outColor = vec4(color, 1.0);
}