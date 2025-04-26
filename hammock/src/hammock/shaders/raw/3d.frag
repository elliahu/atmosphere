#version 450

// Inputs
layout (location = 0) in vec2 inUv;

// Outputs
layout (location = 0) out vec4 outColor;

// Uniforms
layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 inverseProjection;
    mat4 view;
    mat4 inverseView;
    vec4 textureDim;
    vec4 lightPosition;
    vec4 baseSkyColor;
    vec4 tissueColor;
    vec4 fatColor;
    vec4 boneColor;
    vec4 cameraPosition;
} data;

layout (set = 0, binding = 1) uniform sampler3D volumeSampler;

// Push constants
layout (push_constant) uniform PushConstants {
    float resX;
    float resY;
    float elapsedTime;
    float maxSteps;
    float marchSize;
    float airFactor;
    float tissueFactor;
    float fatFactor;
    float depth;
} push;


void main()
{
   float color = texture(volumeSampler, vec3(inUv, push.depth)).r;
   outColor = vec4(color,color,color,1.0);
}
