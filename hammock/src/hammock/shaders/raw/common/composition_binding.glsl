#ifndef COMPOSITION_BINDING_GLSLifndef
#define COMPOSITION_BINDING_GLSL

layout (set = 2, binding = 0) uniform sampler2D positionSampler;
layout (set = 2, binding = 1) uniform sampler2D albedoSampler;
layout (set = 2, binding = 2) uniform sampler2D normalSampler;
layout (set = 2, binding = 3) uniform sampler2D materialPropertySampler;
layout (set = 2, binding = 4) uniform sampler2D accumulationSampler;
layout (set = 2, binding = 5) uniform sampler2D transparencyWeightSampler;

#endif