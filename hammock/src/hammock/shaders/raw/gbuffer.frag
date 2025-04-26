#version 450
#extension GL_EXT_nonuniform_qualifier: enable

#include "common/global_binding.glsl"
#include "common/projection_binding.glsl"
#include "common/mesh_binding.glsl"
#include "common/consts.glsl"

// inputs
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inTangent;

// outputs
layout (location = 0) out vec4 positionBuffer;
layout (location = 1) out vec4 albedoBuffer;
layout (location = 2) out vec4 normalBuffer;
layout (location = 3) out vec4 materialBuffer;

layout (constant_id = 1) const float nearPlane = 0.1;
layout (constant_id = 2) const float farPlane = 64.0;

float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    vec4 position = vec4(inPosition, linearDepth(gl_FragCoord.z));

    vec4 albedo = (global.baseColorTextureIndexes[push.meshIndex] == INVALID_TEXTURE) ?
    global.baseColorFactors[push.meshIndex] : texture(textures[global.baseColorTextureIndexes[push.meshIndex]], inUv);

    float roughness = (global.metallicRoughnessTextureIndexes[push.meshIndex] == INVALID_TEXTURE) ?
    global.metallicRoughnessAlphaCutOffFactors[push.meshIndex].g : texture(textures[global.metallicRoughnessTextureIndexes[push.meshIndex]], inUv).g;

    float metallic = (global.metallicRoughnessTextureIndexes[push.meshIndex] == INVALID_TEXTURE) ?
    global.metallicRoughnessAlphaCutOffFactors[push.meshIndex].r : texture(textures[global.metallicRoughnessTextureIndexes[push.meshIndex]], inUv).b;

    float ambientOcclusion = (global.occlusionTextureIndexes[push.meshIndex] == INVALID_TEXTURE) ?
    1.0 : texture(textures[global.occlusionTextureIndexes[push.meshIndex]], inUv).r;

    positionBuffer = position;

    if (albedo.a < global.metallicRoughnessAlphaCutOffFactors[push.meshIndex].b)
    {
        discard;
    }
    else{
        albedoBuffer = albedo;
    }

    materialBuffer = vec4(roughness, metallic, ambientOcclusion, global.visibilityFlags[push.meshIndex] & VISIBILITY_OPAQUE);
    if (global.normalTextureIndexes[push.meshIndex] != INVALID_TEXTURE)
    {
        vec3 T = normalize(inTangent).xyz;
        vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
        mat3 TBN = mat3(T, B, inNormal);
        normalBuffer = vec4(TBN * normalize(texture(textures[global.normalTextureIndexes[push.meshIndex]], inUv).xyz * 2.0 - 1.0), 1.0);
    }
    else
    {
        normalBuffer = vec4(inNormal, 1.0);
    }
}