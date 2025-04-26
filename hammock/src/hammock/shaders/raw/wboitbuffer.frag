#version 450
#extension GL_EXT_nonuniform_qualifier: enable

#include "common/global_binding.glsl"
#include "common/projection_binding.glsl"
#include "common/mesh_binding.glsl"
#include "common/consts.glsl"
#include "common/functions.glsl"

// inputs
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inTangent;

// outputs
layout (location = 0) out vec4 accumulationBuffer;
layout (location = 1) out float transparencyWeightBuffer;


layout (constant_id = 1) const float nearPlane = 0.1;
layout (constant_id = 2) const float farPlane = 64.0;


void main()
{
    vec4 albedo = (global.baseColorTextureIndexes[push.meshIndex] == INVALID_TEXTURE) ?
    global.baseColorFactors[push.meshIndex] : texture(textures[global.baseColorTextureIndexes[push.meshIndex]], inUv);

    // Opacity calculation
    float opacity = albedo.a; // Use the alpha channel of the base color texture or factor
    float depthWeight = exp(-0.01 * linearDepth(gl_FragCoord.z, nearPlane, farPlane)); // Example scaling factor
    float weight = opacity * depthWeight;
    // Write to accumulation and transparency weight buffers
    accumulationBuffer = vec4(albedo.rgb * weight, 1.0); // Store weighted color contribution
    transparencyWeightBuffer = weight; // Store transparency weight
}