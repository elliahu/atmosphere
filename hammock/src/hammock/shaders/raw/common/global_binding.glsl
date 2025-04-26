#ifndef GLOBAL_BINDING_GLSL
#define GLOBAL_BINDING_GLSL


#include "consts.glsl"


layout(std140, binding = 0) uniform GlobalBuffer {
    vec4 baseColorFactors[MAX_MESHES];
    vec4 metallicRoughnessAlphaCutOffFactors[MAX_MESHES];

    int baseColorTextureIndexes[MAX_MESHES];
    int normalTextureIndexes[MAX_MESHES];
    int metallicRoughnessTextureIndexes[MAX_MESHES];
    int occlusionTextureIndexes[MAX_MESHES];
    int visibilityFlags[MAX_MESHES];
} global;

layout (set = 0, binding = 1) uniform sampler2D textures[];
layout (set = 0, binding = 2) uniform sampler2D environmentSampler;
layout (set = 0, binding = 3) uniform sampler2D prefilteredSampler;
layout (set = 0, binding = 4) uniform sampler2D brdfLUTSampler;
layout (set = 0, binding = 5) uniform sampler2D irradinaceSampler;
#endif